#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/hart/l1.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_MEM_WORDS 256
#define TB_L1_STG_FIFO_DEPTH 8u
#define TB_L1_OST_DEPTH 16u

typedef struct l1_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t bti_req;
    itf_t bti_rsp;
    itf_t axi_aw;
    itf_t axi_w;
    itf_t axi_b;
    itf_t axi_ar;
    itf_t axi_r;
    itf_t flush;
    itf_t flush_ack;

    l1_t dut;
    u32 mem[TB_MEM_WORDS];
    bool stall_axi_r;

    bool rd_active;
    u32 rd_addr;
    u8 rd_id;
    u8 rd_len;
    u8 rd_size;
    u8 rd_burst;
    u8 rd_cnt;
    u32 ar_count;

    bool wr_active;
    bool b_pending;
    u32 wr_addr;
    u8 wr_id;
    u8 wr_size;
    u8 wr_burst;
    u8 wr_cnt;
    u32 aw_count;

    ut_sbd_t sbd;
} l1_tb_t;

static u32 tb_next_addr(u32 addr, u8 size, u8 burst)
{
    return burst == 0 ? addr : addr + (1u << size);
}

static void tb_construct(l1_tb_t *tb, const char *name, const l1_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, bti_req, 2);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp, 2);
    AXI4_AW_IF_CONSTRUCT(tb, axi_aw, 2);
    AXI4_W_IF_CONSTRUCT(tb, axi_w, 16);
    AXI4_B_IF_CONSTRUCT(tb, axi_b, 2);
    AXI4_AR_IF_CONSTRUCT(tb, axi_ar, 2);
    AXI4_R_IF_CONSTRUCT(tb, axi_r, 16);
    L1_FLUSH_IF_CONSTRUCT(tb, flush, 1);
    L1_FLUSH_ACK_IF_CONSTRUCT(tb, flush_ack, 1);

    tb->dut.mod.cycle = tb->mod.cycle;
    tb->dut.bti_req_slv = &tb->bti_req;
    tb->dut.bti_rsp_mst = &tb->bti_rsp;
    tb->dut.axi4_aw_mst = &tb->axi_aw;
    tb->dut.axi4_w_mst = &tb->axi_w;
    tb->dut.axi4_b_slv = &tb->axi_b;
    tb->dut.axi4_ar_mst = &tb->axi_ar;
    tb->dut.axi4_r_slv = &tb->axi_r;
    tb->dut.flush_slv = &tb->flush;
    tb->dut.flush_ack_mst = &tb->flush_ack;
    tb->dut.mod.cycle = tb->mod.cycle;
    l1_construct(&tb->dut, tb->mod.hier_name, "u_dut", conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(l1_tb_t *tb)
{
    l1_reset(&tb->dut);
    dbg_vcd_reset();
    tb->rd_active = false;
    tb->wr_active = false;
    tb->b_pending = false;
    tb->stall_axi_r = false;
    tb->ar_count = 0;
    tb->aw_count = 0;
    for (u32 i = 0; i < TB_MEM_WORDS; i++) {
        tb->mem[i] = 0xa0000000u + i;
    }
}

static void tb_free(l1_tb_t *tb)
{
    l1_free(&tb->dut);
    itf_free(&tb->bti_req);
    itf_free(&tb->bti_rsp);
    itf_free(&tb->axi_aw);
    itf_free(&tb->axi_w);
    itf_free(&tb->axi_b);
    itf_free(&tb->axi_ar);
    itf_free(&tb->axi_r);
    itf_free(&tb->flush);
    itf_free(&tb->flush_ack);
}

static void tb_mock_axi(l1_tb_t *tb)
{
    if (tb->b_pending && !itf_fifo_full(&tb->axi_b)) {
        axi4_b_if_t b = { .id = tb->wr_id, .resp = AXI4_B_RESP_OKAY };
        itf_write(&tb->axi_b, &b);
        tb->b_pending = false;
    }

    if (tb->rd_active && !tb->stall_axi_r && !itf_fifo_full(&tb->axi_r)) {
        u32 word_idx = tb->rd_addr / 4u;
        axi4_r_if_t r = {
            .id = tb->rd_id,
            .data = tb->mem[word_idx],
            .resp = AXI4_R_RESP_OKAY,
            .last = tb->rd_cnt == tb->rd_len
        };
        itf_write(&tb->axi_r, &r);
        if (r.last) {
            tb->rd_active = false;
        } else {
            tb->rd_addr = tb_next_addr(tb->rd_addr, tb->rd_size, tb->rd_burst);
            tb->rd_cnt++;
        }
    }

    if (!tb->rd_active && !itf_fifo_empty(&tb->axi_ar)) {
        axi4_ar_if_t ar;
        itf_read(&tb->axi_ar, &ar);
        tb->ar_count++;
        tb->rd_active = true;
        tb->rd_addr = ar.addr;
        tb->rd_id = ar.id;
        tb->rd_len = ar.len;
        tb->rd_size = ar.size;
        tb->rd_burst = ar.burst;
        tb->rd_cnt = 0;
    }

    if (tb->wr_active && !itf_fifo_empty(&tb->axi_w)) {
        axi4_w_if_t w;
        itf_read(&tb->axi_w, &w);
        u32 word_idx = tb->wr_addr / 4u;
        u32 old = tb->mem[word_idx];
        u32 mask = 0;
        for (u32 i = 0; i < 4; i++) {
            if (w.strb & (1u << i)) {
                mask |= 0xffu << (i * 8u);
            }
        }
        tb->mem[word_idx] = (old & ~mask) | (w.data & mask);
        if (w.last) {
            tb->wr_active = false;
            tb->b_pending = true;
        } else {
            tb->wr_addr = tb_next_addr(tb->wr_addr, tb->wr_size, tb->wr_burst);
            tb->wr_cnt++;
        }
    }

    if (!tb->wr_active && !itf_fifo_empty(&tb->axi_aw)) {
        axi4_aw_if_t aw;
        itf_read(&tb->axi_aw, &aw);
        tb->aw_count++;
        tb->wr_active = true;
        tb->wr_addr = aw.addr;
        tb->wr_id = aw.id;
        tb->wr_size = aw.size;
        tb->wr_burst = aw.burst;
        tb->wr_cnt = 0;
    }
}

static void tb_clock(l1_tb_t *tb)
{
    l1_clock(&tb->dut);
    tb_mock_axi(tb);
    itf_dbg_clock(&tb->bti_req);
    itf_dbg_clock(&tb->bti_rsp);
    itf_dbg_clock(&tb->axi_aw);
    itf_dbg_clock(&tb->axi_w);
    itf_dbg_clock(&tb->axi_b);
    itf_dbg_clock(&tb->axi_ar);
    itf_dbg_clock(&tb->axi_r);
    itf_dbg_clock(&tb->flush);
    itf_dbg_clock(&tb->flush_ack);
    (*tb->cycle)++;
    dbg_vcd_clock();
}

static bool tb_cond_bti_rsp(l1_tb_t *tb)
{
    return !itf_fifo_empty(&tb->bti_rsp);
}

static void tb_bti_read_size(l1_tb_t *tb, u16 trans_id, u32 addr, bti_req_size_t size)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_READ,
        .addr = addr,
        .size = size,
        .data = 0,
        .strobe = 0xf
    };
    itf_write(&tb->bti_req, &req);
}

static void tb_bti_read(l1_tb_t *tb, u16 trans_id, u32 addr)
{
    tb_bti_read_size(tb, trans_id, addr, BTI_REQ_SIZE_B4);
}

static void tb_bti_write_size(l1_tb_t *tb, u16 trans_id, u32 addr, u32 data,
    u8 strobe, bti_req_size_t size)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_WRITE,
        .addr = addr,
        .size = size,
        .data = data,
        .strobe = strobe
    };
    itf_write(&tb->bti_req, &req);
}

static void tb_bti_write(l1_tb_t *tb, u16 trans_id, u32 addr, u32 data, u8 strobe)
{
    tb_bti_write_size(tb, trans_id, addr, data, strobe, BTI_REQ_SIZE_B4);
}

static u32 tb_read_bytes(const l1_tb_t *tb, u32 addr, u32 size)
{
    const u8 *mem = (const u8 *)tb->mem;
    u32 data = 0;
    for (u32 i = 0; i < size; i++) {
        data |= (u32)mem[addr + i] << (i * 8u);
    }
    return data;
}

static bool tb_pop_rsp(l1_tb_t *tb, u16 trans_id, u32 data, bool ok)
{
    if (itf_fifo_empty(&tb->bti_rsp)) {
        return false;
    }
    bti_rsp_if_t rsp;
    itf_read(&tb->bti_rsp, &rsp);
    return rsp.trans_id == trans_id && rsp.data == data && rsp.ok == ok;
}

static void tb_flush(l1_tb_t *tb)
{
    l1_flush_if_t flush = {};
    itf_write(&tb->flush, &flush);
}

static void tb_setup(l1_tb_t *tb, const l1_conf_t *conf)
{
    tb_construct(tb, "l1_tb", conf);
    tb_reset(tb);
}

TEST_CASE(l1_tb_t, read_miss_then_hit)
{
    tb_reset(tb);
    TEST_BEGIN("Read Miss then Hit");

    tb_bti_read(tb, 1, 0x20);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0x20 / 4], true), "miss read returns memory data");
    REQUIRE(tb->ar_count == 1, "one AXI refill read");

    tb_bti_read(tb, 2, 0x24);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, tb->mem[0x24 / 4], true), "same line read hits");
    REQUIRE(tb->ar_count == 1, "hit does not issue another AXI read");

    TEST_END();
}

TEST_CASE(l1_tb_t, pipelined_hits)
{
    enum { REQ_NUM = 8 };

    tb_reset(tb);
    TEST_BEGIN("Pipelined Hits");

    tb_bti_read(tb, 1, 0x00);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0], true),
        "initial read fills the cache line");

    u32 req_num = 0;
    u32 rsp_num = 0;
    u64 last_rsp_cycle = 0;
    bool ordered = true;
    bool data_ok = true;
    bool contiguous = true;
    for (u32 cycle = 0; cycle < UT_TIMEOUT && rsp_num < REQ_NUM; cycle++) {
        if (req_num < REQ_NUM && !itf_fifo_full(&tb->bti_req)) {
            tb_bti_read(tb, (u16)(2u + req_num), (req_num % 16u) * 4u);
            req_num++;
        }

        tb_clock(tb);
        if (itf_fifo_empty(&tb->bti_rsp)) {
            continue;
        }

        bti_rsp_if_t rsp;
        itf_read(&tb->bti_rsp, &rsp);
        ordered &= rsp.trans_id == 2u + rsp_num;
        data_ok &= rsp.ok && rsp.data == tb->mem[rsp_num % 16u];
        if (rsp_num != 0) {
            contiguous &= *tb->cycle == last_rsp_cycle + 1u;
        }
        last_rsp_cycle = *tb->cycle;
        rsp_num++;
    }

    REQUIRE(req_num == REQ_NUM && rsp_num == REQ_NUM,
        "all pipelined hit requests complete");
    REQUIRE(ordered, "pipelined hit responses preserve request order");
    REQUIRE(data_ok, "pipelined hit responses return the expected data");
    REQUIRE(contiguous, "steady-state hit responses retire every cycle");
    REQUIRE(tb->ar_count == 1, "pipelined hits do not issue another refill");

    TEST_END();
}

TEST_CASE(l1_tb_t, pipelined_read_after_write_hits)
{
    static const bti_req_cmd_t cmds[] = {
        BTI_REQ_CMD_WRITE, BTI_REQ_CMD_READ,
        BTI_REQ_CMD_WRITE, BTI_REQ_CMD_READ,
        BTI_REQ_CMD_WRITE, BTI_REQ_CMD_READ
    };
    static const u32 write_data[] = {
        0x11223344u, 0,
        0x55667788u, 0,
        0xa5a55a5au, 0
    };
    static const u32 expected_data[] = {
        0, 0x11223344u,
        0, 0x55667788u,
        0, 0xa5a55a5au
    };
    const u32 req_num_max = sizeof(cmds) / sizeof(cmds[0]);

    tb_reset(tb);
    TEST_BEGIN("Pipelined Read-after-Write Hits");

    tb_bti_read(tb, 1, 0x00);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0], true),
        "initial read fills the cache line");

    u32 req_num = 0;
    u32 rsp_num = 0;
    bool ordered = true;
    bool data_ok = true;
    for (u32 cycle = 0; cycle < UT_TIMEOUT && rsp_num < req_num_max;
        cycle++) {
        if (req_num < req_num_max && !itf_fifo_full(&tb->bti_req)) {
            if (cmds[req_num] == BTI_REQ_CMD_WRITE) {
                tb_bti_write(tb, (u16)(2u + req_num), 0x00,
                    write_data[req_num], 0xf);
            } else {
                tb_bti_read(tb, (u16)(2u + req_num), 0x00);
            }
            req_num++;
        }

        tb_clock(tb);
        if (itf_fifo_empty(&tb->bti_rsp)) {
            continue;
        }

        bti_rsp_if_t rsp;
        itf_read(&tb->bti_rsp, &rsp);
        ordered &= rsp.trans_id == 2u + rsp_num;
        data_ok &= rsp.ok && rsp.data == expected_data[rsp_num];
        rsp_num++;
    }

    REQUIRE(req_num == req_num_max && rsp_num == req_num_max,
        "all pipelined read-after-write requests complete");
    REQUIRE(ordered, "read-after-write responses preserve request order");
    REQUIRE(data_ok, "each read observes the immediately preceding write");
    REQUIRE(tb->ar_count == 1,
        "pipelined read-after-write hits do not refill");

    TEST_END();
}

TEST_CASE(l1_tb_t, writeback_on_dirty_evict)
{
    tb_reset(tb);
    TEST_BEGIN("Writeback on Dirty Evict");

    tb_bti_read(tb, 1, 0x00);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0], true), "line 0 filled");

    tb_bti_write(tb, 2, 0x04, 0x12345678, 0xf);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, 0, true), "write hit accepted");
    REQUIRE(tb->mem[1] != 0x12345678, "memory not updated before eviction");

    tb_bti_read(tb, 3, 0x40);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 3, tb->mem[0x40 / 4], true), "conflict line filled");
    REQUIRE(tb->aw_count == 1, "dirty victim wrote back once");
    REQUIRE(tb->mem[1] == 0x12345678, "dirty word written back to memory");

    TEST_END();
}

TEST_CASE(l1_tb_t, unaligned_reads)
{
    tb_reset(tb);
    TEST_BEGIN("Unaligned Reads");

    tb_bti_read_size(tb, 1, 0x21, BTI_REQ_SIZE_B1);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb_read_bytes(tb, 0x21, 1), true),
        "byte read is aligned to response bit 0");

    tb_bti_read_size(tb, 2, 0x23, BTI_REQ_SIZE_B2);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, tb_read_bytes(tb, 0x23, 2), true),
        "halfword read crosses a cache word");

    tb_bti_read_size(tb, 3, 0x25, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 3, tb_read_bytes(tb, 0x25, 4), true),
        "word read crosses cache words");
    REQUIRE(tb->ar_count == 1, "unaligned reads in one line share one refill");

    TEST_END();
}

TEST_CASE(l1_tb_t, cross_line_read_write)
{
    tb_reset(tb);
    TEST_BEGIN("Cross-line Read and Write");

    tb_bti_read_size(tb, 1, 0x3f, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb_read_bytes(tb, 0x3f, 4), true),
        "word read is assembled from two cache lines");
    REQUIRE(tb->ar_count == 2, "cross-line read refills both lines");

    tb_bti_write_size(tb, 2, 0x3f, 0x78563412, 0xf, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, 0, true), "cross-line write completes once");

    tb_bti_read_size(tb, 3, 0x3f, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 3, 0x78563412, true),
        "cross-line write updates bytes in both lines");

    TEST_END();
}

TEST_CASE(l1_tb_t, bypass_range)
{
    tb_reset(tb);
    TEST_BEGIN("Bypass Range");

    tb_bti_read(tb, 1, 0x80);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0x80 / 4], true), "bypass read returns memory data");
    REQUIRE(tb->ar_count == 1, "bypass read issues one AXI read");

    tb_bti_read(tb, 2, 0x84);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, tb->mem[0x84 / 4], true), "second bypass read returns memory data");
    REQUIRE(tb->ar_count == 2, "bypass read does not allocate");

    TEST_END();
}

TEST_CASE(l1_tb_t, bypass_unaligned_read_write)
{
    tb_reset(tb);
    TEST_BEGIN("Bypass Unaligned Read and Write");

    tb_bti_read_size(tb, 1, 0x83, BTI_REQ_SIZE_B2);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb_read_bytes(tb, 0x83, 2), true),
        "bypass halfword read crosses an AXI word");

    tb_bti_write_size(tb, 2, 0x83, 0x44332211, 0xf, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, 0, true),
        "bypass word write crossing an AXI word completes once");
    REQUIRE(tb_read_bytes(tb, 0x83, 4) == 0x44332211,
        "bypass split write updates the addressed bytes");

    tb_bti_read_size(tb, 3, 0x83, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 3, 0x44332211, true),
        "bypass split read observes the split write data");

    TEST_END();
}

TEST_CASE(l1_tb_t, bypass_split_multi_outstanding)
{
    tb_reset(tb);
    TEST_BEGIN("Bypass Split Multi Outstanding");

    tb->stall_axi_r = true;
    tb_bti_read_size(tb, 1, 0x83, BTI_REQ_SIZE_B4);
    RUN_CYCLES(2);

    tb_bti_read(tb, 2, 0x88);
    RUN_CYCLES(4);
    REQUIRE(tb->dut.ost.count == 2,
        "split bypass read and younger bypass read are both outstanding");
    REQUIRE(itf_fifo_empty(&tb->bti_req),
        "younger bypass read is accepted while older split read waits");
    REQUIRE(itf_fifo_empty(&tb->bti_rsp),
        "no BTI response is returned before AXI responses arrive");

    tb->stall_axi_r = false;
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb_read_bytes(tb, 0x83, 4), true),
        "older split bypass read returns first");
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, tb->mem[0x88 / 4], true),
        "younger bypass read returns second");

    TEST_END();
}

TEST_CASE(l1_tb_t, flush_invalidates)
{
    tb_reset(tb);
    TEST_BEGIN("Flush Invalidates");

    tb_bti_read(tb, 1, 0x20);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0x20 / 4], true), "initial read fills line");
    REQUIRE(tb->ar_count == 1, "one refill before flush");

    tb_flush(tb);
    RUN_CYCLES(1);

    tb_bti_read(tb, 2, 0x20);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, tb->mem[0x20 / 4], true), "read after flush returns data");
    REQUIRE(tb->ar_count == 2, "read after flush refills again");

    TEST_END();
}

TEST_CASE(l1_tb_t, hit_under_miss_uses_ost)
{
    tb_reset(tb);
    TEST_BEGIN("Hit Under Miss Uses OST");

    tb_bti_read(tb, 1, 0x00);
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 1, tb->mem[0], true), "line 0 filled");

    tb->stall_axi_r = true;
    tb_bti_read(tb, 2, 0x40);
    RUN_CYCLES(4);
    REQUIRE(tb->dut.state != L1_STATE_IDLE, "miss is active");
    REQUIRE(itf_fifo_empty(&tb->bti_rsp), "older miss has no response yet");

    tb_bti_read(tb, 3, 0x00);
    RUN_CYCLES(4);
    REQUIRE(tb->dut.ost.count == 2, "hit under miss occupies another ost slot");
    REQUIRE(itf_fifo_empty(&tb->bti_req), "hit under miss request was accepted");
    REQUIRE(itf_fifo_empty(&tb->bti_rsp), "younger hit waits behind older miss");

    tb->stall_axi_r = false;
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 2, tb->mem[0x40 / 4], true),
        "older miss response returns first");
    RUN_POLL_UNTIL(tb_cond_bti_rsp, UT_TIMEOUT);
    REQUIRE(tb_pop_rsp(tb, 3, tb->mem[0], true),
        "younger hit response returns second");

    TEST_END();
}

int main(void)
{
    l1_conf_t cached_conf = {
        .full_bypass = false,
        .ro = false,
        .size = L1_LINE_SIZE,
        .way_num = 1,
        .latency = 1,
        .stg_fifo_depth = TB_L1_STG_FIFO_DEPTH,
        .ost_depth = TB_L1_OST_DEPTH
    };
    l1_tb_t tb;
    tb_setup(&tb, &cached_conf);
    TEST_RUN(read_miss_then_hit);
    TEST_RUN(pipelined_hits);
    TEST_RUN(pipelined_read_after_write_hits);
    TEST_RUN(writeback_on_dirty_evict);
    TEST_RUN(unaligned_reads);
    TEST_RUN(cross_line_read_write);
    TEST_RUN(flush_invalidates);
    int ret = ut_sbd_ret(&tb.sbd);
    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);
    if (ret != 0) {
        return ret;
    }

    l1_conf_t hit_under_miss_conf = {
        .full_bypass = false,
        .ro = false,
        .size = L1_LINE_SIZE * 2,
        .way_num = 1,
        .latency = 1,
        .stg_fifo_depth = TB_L1_STG_FIFO_DEPTH,
        .ost_depth = TB_L1_OST_DEPTH
    };
    tb_setup(&tb, &hit_under_miss_conf);
    TEST_RUN(hit_under_miss_uses_ost);
    ut_sbd_summary(&tb.sbd);
    ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    if (ret != 0) {
        return ret;
    }

    l1_conf_t bypass_conf = {
        .ro = false,
        .size = L1_LINE_SIZE,
        .way_num = 1,
        .latency = 1,
        .stg_fifo_depth = TB_L1_STG_FIFO_DEPTH,
        .ost_depth = TB_L1_OST_DEPTH,
        .bypass_bases = { 0x80 },
        .bypass_sizes = { 0x40 }
    };
    tb_setup(&tb, &bypass_conf);
    TEST_RUN(bypass_range);
    TEST_RUN(bypass_unaligned_read_write);
    TEST_RUN(bypass_split_multi_outstanding);
    ut_sbd_summary(&tb.sbd);
    ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
