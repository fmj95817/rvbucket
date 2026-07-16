#include <stdio.h>
#include <string.h>
#include "core/l2.h"
#include "mem/ram.h"
#include "dbg/vcd.h"
#include "spec/core/cache.h"
#include "utils.h"

#define TB_MEM_SIZE 4096u
#define TB_FIFO_DEPTH 32u
#define TB_TIMEOUT 20000u
#define TB_L2_STG_FIFO_DEPTH 8u
#define TB_L2_BYPASS_OST_DEPTH 16u

typedef struct l2_tb {
    mod_t mod;
    u64 cycle_val;

    AXI4_IF_DECL(h0_);
    AXI4_IF_DECL(h1_);
    AXI4_IF_DECL(mem_);

    l2_t dut;
    ram_t ram;
    bool stall_ram;
    ut_sbd_t sbd;
} l2_tb_t;

static void tb_construct_with_conf(l2_tb_t *tb, const char *name,
    const l2_conf_t *conf)
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
    l2_construct(&tb->dut, tb->mod.hier_name, "u_dut", conf);

    AXI4_SLV_CONNECT(&tb->ram, , tb, mem_);
    tb->ram.mod.cycle = tb->mod.cycle;
    ram_construct(&tb->ram, tb->mod.hier_name, "u_ram", 1, RAM_MODE_AXI,
        TB_MEM_SIZE, 0, 1);
    ut_sbd_init(&tb->sbd);
}

static void tb_construct(l2_tb_t *tb, const char *name)
{
    l2_conf_t conf = {
        .size = L2_LINE_SIZE,
        .way_num = 1,
        .latency = 1,
        .stg_fifo_depth = TB_L2_STG_FIFO_DEPTH,
        .bypass_ost_depth = TB_L2_BYPASS_OST_DEPTH
    };
    tb_construct_with_conf(tb, name, &conf);
}

static void tb_reset(l2_tb_t *tb)
{
    AXI4_IF_RESET(tb, h0_);
    AXI4_IF_RESET(tb, h1_);
    AXI4_IF_RESET(tb, mem_);
    l2_reset(&tb->dut);
    ram_reset(&tb->ram);
    tb->stall_ram = false;
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
    if (!tb->stall_ram) {
        ram_clock(&tb->ram);
    }
    AXI4_IF_DBG_CLOCK(tb, h0_);
    AXI4_IF_DBG_CLOCK(tb, h1_);
    AXI4_IF_DBG_CLOCK(tb, mem_);
    tb->cycle_val++;
    dbg_vcd_clock();
}

static void tb_send_read_user(l2_tb_t *tb, u32 host, u8 id, u32 addr, u8 len,
    u8 cache, u32 user)
{
    axi4_ar_if_t ar = {
        .id = id,
        .addr = addr,
        .len = len,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .cache = cache,
        .user = user
    };
    itf_write(host == 0 ? &tb->h0_axi4_ar_itf : &tb->h1_axi4_ar_itf, &ar);
}

static void tb_send_read(l2_tb_t *tb, u32 host, u8 id, u32 addr, u8 len, u8 cache)
{
    tb_send_read_user(tb, host, id, addr, len, cache, 0);
}

static void tb_send_cmo(l2_tb_t *tb, u32 host, u8 id, u32 addr, u32 user)
{
    axi4_ar_if_t ar = {
        .id = id,
        .addr = addr,
        .len = 0,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_FIXED,
        .cache = 0xf,
        .user = user
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

static bool tb_collect_single_r(l2_tb_t *tb, u32 host, u8 id, u32 data)
{
    itf_t *r_itf = host == 0 ? &tb->h0_axi4_r_itf : &tb->h1_axi4_r_itf;
    for (u32 cycle = 0; cycle < TB_TIMEOUT && !tb_read_rsp_ready(tb, host); cycle++) {
        tb_clock(tb);
    }
    if (itf_fifo_empty(r_itf)) {
        return false;
    }

    axi4_r_if_t r;
    itf_read(r_itf, &r);
    return r.id == id && r.data == data && r.resp == AXI4_R_RESP_OKAY && r.last;
}

static bool tb_collect_write_b(l2_tb_t *tb, u32 host, u8 id)
{
    itf_t *b_itf = host == 0 ? &tb->h0_axi4_b_itf : &tb->h1_axi4_b_itf;
    for (u32 cycle = 0; cycle < TB_TIMEOUT && !tb_write_rsp_ready(tb, host); cycle++) {
        tb_clock(tb);
    }
    if (itf_fifo_empty(b_itf)) {
        return false;
    }

    axi4_b_if_t b;
    itf_read(b_itf, &b);
    return b.id == id && b.resp == AXI4_B_RESP_OKAY;
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

TEST_CASE(l2_tb_t, cmo_clean_writes_back_and_keeps_line)
{
    tb_reset(tb);
    TEST_BEGIN("CMO Clean Writes Back and Keeps Line");

    tb_send_write(tb, 1, 7, 0, 0x55667788u);
    REQUIRE(tb_collect_write_b(tb, 1, 7), "cached write completes");
    REQUIRE(((u32 *)tb->ram.data)[0] != 0x55667788u, "dirty data remains in L2");

    tb_send_cmo(tb, 1, 8, 0, AXI4_USER_CMO_CLEAN);
    REQUIRE(tb_collect_single_r(tb, 1, 8, 0), "cbo.clean response completes");
    REQUIRE(((u32 *)tb->ram.data)[0] == 0x55667788u, "clean writes back dirty L2 line");

    ((u32 *)tb->ram.data)[0] = 0xaabbccddu;
    tb_send_read(tb, 1, 9, 0, 0, 0xf);
    REQUIRE(tb_collect_single_r(tb, 1, 9, 0x55667788u),
        "clean keeps L2 line valid");

    TEST_END();
}

TEST_CASE(l2_tb_t, cmo_flush_writes_back_and_invalidates)
{
    tb_reset(tb);
    TEST_BEGIN("CMO Flush Writes Back and Invalidates");

    tb_send_write(tb, 1, 10, 0, 0x01020304u);
    REQUIRE(tb_collect_write_b(tb, 1, 10), "cached write completes");

    tb_send_cmo(tb, 1, 11, 0, AXI4_USER_CMO_FLUSH);
    REQUIRE(tb_collect_single_r(tb, 1, 11, 0), "cbo.flush response completes");
    REQUIRE(((u32 *)tb->ram.data)[0] == 0x01020304u, "flush writes back dirty L2 line");

    ((u32 *)tb->ram.data)[0] = 0x11223344u;
    tb_send_read(tb, 1, 12, 0, 0, 0xf);
    REQUIRE(tb_collect_single_r(tb, 1, 12, 0x11223344u),
        "read after flush refills from memory");

    TEST_END();
}

TEST_CASE(l2_tb_t, cmo_inval_discards_dirty_line)
{
    tb_reset(tb);
    TEST_BEGIN("CMO Invalidate Discards Dirty Line");

    u32 original = ((u32 *)tb->ram.data)[0];
    tb_send_write(tb, 1, 13, 0, 0x89abcdefu);
    REQUIRE(tb_collect_write_b(tb, 1, 13), "cached write completes");

    tb_send_cmo(tb, 1, 14, 0, AXI4_USER_CMO_INVAL);
    REQUIRE(tb_collect_single_r(tb, 1, 14, 0), "cbo.inval response completes");
    REQUIRE(((u32 *)tb->ram.data)[0] == original, "invalidate does not write back dirty line");

    tb_send_read(tb, 1, 15, 0, 0, 0xf);
    REQUIRE(tb_collect_single_r(tb, 1, 15, original),
        "read after invalidate refills old memory data");

    TEST_END();
}

TEST_CASE(l2_tb_t, cmo_rsvd_bits_do_not_match)
{
    tb_reset(tb);
    TEST_BEGIN("CMO Reserved Bits Do Not Match");

    u32 addr = 192;
    u32 expected;
    u32 user = AXI4_USER_CMO_CLEAN | (1u << 8);
    memcpy(&expected, tb->ram.data + addr, sizeof(expected));

    REQUIRE(!axi4_user_is_cmo(user), "non-zero reserved bits are not CMO");
    tb_send_read_user(tb, 1, 16, addr, 0, 0xf, user);
    REQUIRE(tb_collect_single_r(tb, 1, 16, expected),
        "request with reserved user bits is handled as a normal read");

    TEST_END();
}

TEST_CASE(l2_tb_t, other_port_hit_under_miss)
{
    tb_reset(tb);
    TEST_BEGIN("Other Port Hit Under Miss");

    tb_send_read(tb, 1, 1, 0, 0, 0xf);
    REQUIRE(tb_collect_read(tb, 1, 1, 0, 1), "host 1 line 0 filled");

    tb->stall_ram = true;
    tb_send_read(tb, 0, 2, L2_LINE_SIZE, 0, 0xf);
    RUN_CYCLES(8);
    REQUIRE(tb->dut.mem_state != L2_MEM_STATE_IDLE, "host 0 miss is active");
    REQUIRE(!tb_read_rsp_ready(tb, 0), "host 0 miss is still waiting");

    tb_send_read(tb, 1, 3, 0, 0, 0xf);
    RUN_CYCLES(4);
    REQUIRE(tb_read_rsp_ready(tb, 1),
        "host 1 cached hit returns while host 0 miss waits");
    REQUIRE(tb_collect_read(tb, 1, 3, 0, 1),
        "host 1 hit data is correct");

    tb->stall_ram = false;
    REQUIRE(tb_collect_read(tb, 0, 2, L2_LINE_SIZE, 1),
        "host 0 miss completes after memory resumes");

    TEST_END();
}

int main(void)
{
    l2_tb_t tb;
    tb_construct(&tb, "l2_tb");
    TEST_RUN(read_miss_hit_and_arb);
    TEST_RUN(writeback_and_bypass);
    TEST_RUN(cmo_clean_writes_back_and_keeps_line);
    TEST_RUN(cmo_flush_writes_back_and_invalidates);
    TEST_RUN(cmo_inval_discards_dirty_line);
    TEST_RUN(cmo_rsvd_bits_do_not_match);
    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    if (ret != 0) {
        return ret;
    }

    l2_conf_t hit_under_miss_conf = {
        .size = L2_LINE_SIZE * 2,
        .way_num = 1,
        .latency = 1,
        .stg_fifo_depth = TB_L2_STG_FIFO_DEPTH,
        .bypass_ost_depth = TB_L2_BYPASS_OST_DEPTH
    };
    tb_construct_with_conf(&tb, "l2_tb", &hit_under_miss_conf);
    TEST_RUN(other_port_hit_under_miss);
    ut_sbd_summary(&tb.sbd);
    ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
