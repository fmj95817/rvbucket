#include <stdio.h>
#include <string.h>
#include "core/l2.h"
#include "mem/ram.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_MEM_SIZE 4096u
#define TB_FIFO_DEPTH 32u
#define TB_TIMEOUT 20000u

typedef struct l2_tb {
    mod_t mod;
    u64 cycle_val;

    AXI4_IF_DECL(h0_);
    AXI4_IF_DECL(h1_);
    AXI4_IF_DECL(mem_);

    l2_t dut;
    ram_t ram;
    ut_sbd_t sbd;
} l2_tb_t;

static void tb_construct(l2_tb_t *tb, const char *name)
{
    tb->cycle_val = 0;
    tb->mod.cycle = &tb->cycle_val;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);
    DBG_VCD_MODULE_SCOPE(name);

    AXI4_IF_CONSTRUCT(tb, h0_, TB_FIFO_DEPTH);
    AXI4_IF_CONSTRUCT(tb, h1_, TB_FIFO_DEPTH);
    AXI4_IF_CONSTRUCT(tb, mem_, TB_FIFO_DEPTH);

    tb->dut.i_axi4_aw_slv = &tb->h0_axi4_aw_itf;
    tb->dut.i_axi4_w_slv = &tb->h0_axi4_w_itf;
    tb->dut.i_axi4_b_mst = &tb->h0_axi4_b_itf;
    tb->dut.i_axi4_ar_slv = &tb->h0_axi4_ar_itf;
    tb->dut.i_axi4_r_mst = &tb->h0_axi4_r_itf;
    tb->dut.d_axi4_aw_slv = &tb->h1_axi4_aw_itf;
    tb->dut.d_axi4_w_slv = &tb->h1_axi4_w_itf;
    tb->dut.d_axi4_b_mst = &tb->h1_axi4_b_itf;
    tb->dut.d_axi4_ar_slv = &tb->h1_axi4_ar_itf;
    tb->dut.d_axi4_r_mst = &tb->h1_axi4_r_itf;
    tb->dut.mem_axi4_aw_mst = &tb->mem_axi4_aw_itf;
    tb->dut.mem_axi4_w_mst = &tb->mem_axi4_w_itf;
    tb->dut.mem_axi4_b_slv = &tb->mem_axi4_b_itf;
    tb->dut.mem_axi4_ar_mst = &tb->mem_axi4_ar_itf;
    tb->dut.mem_axi4_r_slv = &tb->mem_axi4_r_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    l2_conf_t conf = {
        .size = L2_LINE_SIZE,
        .way_num = 1
    };
    l2_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    AXI4_SLV_CONNECT(&tb->ram, , tb, mem_);
    tb->ram.mod.cycle = tb->mod.cycle;
    ram_construct(&tb->ram, tb->mod.hier_name, "u_ram", 1, RAM_MODE_AXI,
        TB_MEM_SIZE, 0);
    ut_sbd_init(&tb->sbd);
}

static void tb_reset(l2_tb_t *tb)
{
    AXI4_IF_RESET(tb, h0_);
    AXI4_IF_RESET(tb, h1_);
    AXI4_IF_RESET(tb, mem_);
    l2_reset(&tb->dut);
    ram_reset(&tb->ram);
    dbg_vcd_reset();
    for (u32 i = 0; i < TB_MEM_SIZE / sizeof(u32); i++) {
        ((u32 *)tb->ram.data)[i] = 0xa0000000u + i;
    }
}

static void tb_free(l2_tb_t *tb)
{
    l2_free(&tb->dut);
    ram_free(&tb->ram);
    AXI4_IF_FREE(tb, h0_);
    AXI4_IF_FREE(tb, h1_);
    AXI4_IF_FREE(tb, mem_);
    mod_free(&tb->mod);
}

static void tb_clock(l2_tb_t *tb)
{
    l2_clock(&tb->dut);
    ram_clock(&tb->ram);
    AXI4_IF_DBG_CLOCK(tb, h0_);
    AXI4_IF_DBG_CLOCK(tb, h1_);
    AXI4_IF_DBG_CLOCK(tb, mem_);
    tb->cycle_val++;
    dbg_vcd_clock();
}

static void tb_send_read(l2_tb_t *tb, u32 host, u8 id, u32 addr, u8 len, u8 cache)
{
    axi4_ar_if_t ar = {
        .id = id,
        .addr = addr,
        .len = len,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .cache = cache
    };
    itf_write(host == 0 ? &tb->h0_axi4_ar_itf : &tb->h1_axi4_ar_itf, &ar);
}

static void tb_send_write(l2_tb_t *tb, u32 host, u8 id, u32 addr, u32 data)
{
    axi4_aw_if_t aw = {
        .id = id,
        .addr = addr,
        .len = 0,
        .size = AXI4_AW_SIZE_B4,
        .burst = AXI4_AW_BURST_INCR,
        .cache = 0xf
    };
    axi4_w_if_t w = { .data = data, .strb = 0xf, .last = true };
    itf_write(host == 0 ? &tb->h0_axi4_aw_itf : &tb->h1_axi4_aw_itf, &aw);
    itf_write(host == 0 ? &tb->h0_axi4_w_itf : &tb->h1_axi4_w_itf, &w);
}

static bool tb_read_rsp_ready(l2_tb_t *tb, u32 host)
{
    return !itf_fifo_empty(host == 0 ? &tb->h0_axi4_r_itf : &tb->h1_axi4_r_itf);
}

static bool tb_write_rsp_ready(l2_tb_t *tb, u32 host)
{
    return !itf_fifo_empty(host == 0 ? &tb->h0_axi4_b_itf : &tb->h1_axi4_b_itf);
}

static bool tb_collect_read(l2_tb_t *tb, u32 host, u8 id, u32 addr, u32 beat_num)
{
    itf_t *r_itf = host == 0 ? &tb->h0_axi4_r_itf : &tb->h1_axi4_r_itf;
    for (u32 i = 0; i < beat_num; i++) {
        for (u32 cycle = 0; cycle < TB_TIMEOUT && !tb_read_rsp_ready(tb, host); cycle++) {
            tb_clock(tb);
        }
        if (itf_fifo_empty(r_itf)) {
            return false;
        }
        axi4_r_if_t r;
        itf_read(r_itf, &r);
        u32 expected;
        memcpy(&expected, tb->ram.data + addr + i * sizeof(u32), sizeof(expected));
        if (r.id != id || r.data != expected || r.resp != AXI4_R_RESP_OKAY ||
            r.last != (i == beat_num - 1u)) {
            return false;
        }
    }
    return true;
}

TEST_CASE(l2_tb_t, read_miss_hit_and_arb)
{
    tb_reset(tb);
    TEST_BEGIN("Read Miss, Hit and I/D Arbitration");

    tb_send_read(tb, 0, 1, 0, 15, 0xf);
    REQUIRE(tb_collect_read(tb, 0, 1, 0, 16), "host 0 line refill returns all beats");
    REQUIRE(*tb->dut.perf_miss == 1, "first line access misses once");
    REQUIRE(*tb->dut.perf_hit == 15, "remaining refill burst beats hit");

    tb_send_read(tb, 0, 2, 0, 0, 0xf);
    tb_send_read(tb, 1, 3, 4, 0, 0xf);
    tb_clock(tb);
    REQUIRE(!tb_read_rsp_ready(tb, 0) && !tb_read_rsp_ready(tb, 1),
        "both ports accept requests before producing responses");
    tb_clock(tb);
    REQUIRE(tb_read_rsp_ready(tb, 0) && tb_read_rsp_ready(tb, 1),
        "I and D hits complete in the same cycle");
    REQUIRE(tb_collect_read(tb, 0, 2, 0, 1), "host 0 hit completes");
    REQUIRE(tb_collect_read(tb, 1, 3, 4, 1), "host 1 hit completes");
    REQUIRE(*tb->dut.perf_miss == 1, "shared line serves both hosts without refill");

    TEST_END();
}

TEST_CASE(l2_tb_t, writeback_and_bypass)
{
    tb_reset(tb);
    TEST_BEGIN("Dirty Writeback and Non-cacheable Bypass");

    tb_send_write(tb, 1, 4, 0, 0x12345678u);
    for (u32 cycle = 0; cycle < TB_TIMEOUT && !tb_write_rsp_ready(tb, 1); cycle++) {
        tb_clock(tb);
    }
    axi4_b_if_t b;
    itf_read(&tb->h1_axi4_b_itf, &b);
    REQUIRE(b.id == 4 && b.resp == AXI4_B_RESP_OKAY, "cached write completes");
    REQUIRE(((u32 *)tb->ram.data)[0] != 0x12345678u, "dirty data remains in L2");

    tb_send_read(tb, 1, 5, L2_LINE_SIZE, 0, 0xf);
    REQUIRE(tb_collect_read(tb, 1, 5, L2_LINE_SIZE, 1), "conflict line read completes");
    REQUIRE(((u32 *)tb->ram.data)[0] == 0x12345678u, "dirty victim writes back");
    REQUIRE(*tb->dut.perf_writeback == 1, "writeback counter increments");

    u32 hit = *tb->dut.perf_hit;
    u32 miss = *tb->dut.perf_miss;
    tb_send_read(tb, 0, 6, 128, 0, 0);
    REQUIRE(tb_collect_read(tb, 0, 6, 128, 1), "non-cacheable read bypasses");
    REQUIRE(*tb->dut.perf_bypass == 1, "bypass counter increments");
    REQUIRE(*tb->dut.perf_hit == hit && *tb->dut.perf_miss == miss,
        "bypass does not affect cache lookup counters");

    TEST_END();
}

int main(void)
{
    l2_tb_t tb;
    tb_construct(&tb, "l2_tb");
    TEST_RUN(read_miss_hit_and_arb);
    TEST_RUN(writeback_and_bypass);
    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
