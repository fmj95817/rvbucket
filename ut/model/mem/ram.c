#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem/ram.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define RAM_SIZE 0x1000
#define RAM_BASE 0x80000000

typedef struct ram_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t bti_req_itf;
    itf_t bti_rsp_itf;

    itf_t axi4_aw_itf;
    itf_t axi4_w_itf;
    itf_t axi4_b_itf;
    itf_t axi4_ar_itf;
    itf_t axi4_r_itf;

    ram_t dut;

    ut_sbd_t sbd;
} ram_tb_t;

static void tb_construct(ram_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, bti_req_itf, 4);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp_itf, 4);

    AXI4_AW_IF_CONSTRUCT(tb, axi4_aw_itf, 4);
    AXI4_W_IF_CONSTRUCT(tb, axi4_w_itf, 4);
    AXI4_B_IF_CONSTRUCT(tb, axi4_b_itf, 4);
    AXI4_AR_IF_CONSTRUCT(tb, axi4_ar_itf, 4);
    AXI4_R_IF_CONSTRUCT(tb, axi4_r_itf, 4);

    ut_sbd_init(&tb->sbd);
}

static void tb_construct_bti(ram_tb_t *tb)
{
    tb->dut.bti_req_slvs[0] = &tb->bti_req_itf;
    tb->dut.bti_rsp_msts[0] = &tb->bti_rsp_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    ram_construct(&tb->dut, tb->mod.hier_name, "u_ram", 1, RAM_MODE_BTI, RAM_SIZE, RAM_BASE);
}

static void tb_construct_axi(ram_tb_t *tb)
{
    tb->dut.axi4_aw_slv = &tb->axi4_aw_itf;
    tb->dut.axi4_w_slv = &tb->axi4_w_itf;
    tb->dut.axi4_b_mst = &tb->axi4_b_itf;
    tb->dut.axi4_ar_slv = &tb->axi4_ar_itf;
    tb->dut.axi4_r_mst = &tb->axi4_r_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    ram_construct(&tb->dut, tb->mod.hier_name, "u_ram", 1, RAM_MODE_AXI, RAM_SIZE, RAM_BASE);
}

static void tb_dut_reset(ram_tb_t *tb)
{
    ram_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_clock(ram_tb_t *tb)
{
    ram_clock(&tb->dut);

    itf_dbg_clock(&tb->bti_req_itf);
    itf_dbg_clock(&tb->bti_rsp_itf);
    itf_dbg_clock(&tb->axi4_aw_itf);
    itf_dbg_clock(&tb->axi4_w_itf);
    itf_dbg_clock(&tb->axi4_b_itf);
    itf_dbg_clock(&tb->axi4_ar_itf);
    itf_dbg_clock(&tb->axi4_r_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free_dut(ram_tb_t *tb)
{
    ram_free(&tb->dut);
}

static void tb_free(ram_tb_t *tb)
{
    itf_free(&tb->bti_req_itf);
    itf_free(&tb->bti_rsp_itf);
    itf_free(&tb->axi4_aw_itf);
    itf_free(&tb->axi4_w_itf);
    itf_free(&tb->axi4_b_itf);
    itf_free(&tb->axi4_ar_itf);
    itf_free(&tb->axi4_r_itf);
}

static void tb_bti_write_read_req_size(ram_tb_t *tb, u16 trans_id, u32 addr,
    bti_req_size_t size)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_READ,
        .size = size,
        .addr = addr,
        .data = 0,
        .strobe = 0
    };
    itf_write(&tb->bti_req_itf, &req);
}

static void tb_bti_write_read_req(ram_tb_t *tb, u16 trans_id, u32 addr)
{
    tb_bti_write_read_req_size(tb, trans_id, addr, BTI_REQ_SIZE_B4);
}

static void tb_bti_write_write_req_size(ram_tb_t *tb, u16 trans_id, u32 addr,
    u32 data, u8 strobe, bti_req_size_t size)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_WRITE,
        .size = size,
        .addr = addr,
        .data = data,
        .strobe = strobe
    };
    itf_write(&tb->bti_req_itf, &req);
}

static void tb_bti_write_write_req(ram_tb_t *tb, u16 trans_id, u32 addr,
    u32 data, u8 strobe)
{
    tb_bti_write_write_req_size(tb, trans_id, addr, data, strobe, BTI_REQ_SIZE_B4);
}

static bool tb_cond_bti_rsp_ready(ram_tb_t *tb)
{
    return !itf_fifo_empty(&tb->bti_rsp_itf);
}

static bool tb_bti_check_and_pop_rsp(ram_tb_t *tb, u16 expected_trans_id,
                                      u32 expected_data, bool expected_ok)
{
    if (itf_fifo_empty(&tb->bti_rsp_itf)) return false;
    bti_rsp_if_t rsp;
    itf_read(&tb->bti_rsp_itf, &rsp);
    return (rsp.trans_id == expected_trans_id) &&
           (rsp.data == expected_data) &&
           (rsp.ok == expected_ok);
}

static void tb_axi_write_ar(ram_tb_t *tb, u8 id, u32 addr, u8 len,
                             axi4_ar_size_t size, axi4_ar_burst_t burst)
{
    axi4_ar_if_t ar = {
        .id = id, .addr = addr, .len = len, .size = size, .burst = burst,
        .lock = false, .cache = 0, .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->axi4_ar_itf, &ar);
}

static void tb_axi_write_aw(ram_tb_t *tb, u8 id, u32 addr, u8 len,
                             axi4_aw_size_t size, axi4_aw_burst_t burst)
{
    axi4_aw_if_t aw = {
        .id = id, .addr = addr, .len = len, .size = size, .burst = burst,
        .lock = false, .cache = 0, .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->axi4_aw_itf, &aw);
}

static void tb_axi_write_w(ram_tb_t *tb, u32 data, u8 strb, bool last)
{
    axi4_w_if_t w = { .data = data, .strb = strb, .last = last };
    itf_write(&tb->axi4_w_itf, &w);
}

static bool tb_cond_axi_r_ready(ram_tb_t *tb)
{
    return !itf_fifo_empty(&tb->axi4_r_itf);
}

static bool tb_cond_axi_b_ready(ram_tb_t *tb)
{
    return !itf_fifo_empty(&tb->axi4_b_itf);
}

static bool tb_axi_check_and_pop_r(ram_tb_t *tb, u8 expected_id, u32 expected_data,
                                    axi4_r_resp_t expected_resp, bool expected_last)
{
    if (itf_fifo_empty(&tb->axi4_r_itf)) return false;
    axi4_r_if_t r;
    itf_read(&tb->axi4_r_itf, &r);
    return (r.id == expected_id) && (r.data == expected_data) &&
           (r.resp == expected_resp) && (r.last == expected_last);
}

static bool tb_axi_check_and_pop_b(ram_tb_t *tb, u8 expected_id, axi4_b_resp_t expected_resp)
{
    if (itf_fifo_empty(&tb->axi4_b_itf)) return false;
    axi4_b_if_t b;
    itf_read(&tb->axi4_b_itf, &b);
    return (b.id == expected_id) && (b.resp == expected_resp);
}

TEST_CASE(ram_tb_t, bti_read)
{
    TEST_BEGIN("BTI Read");

    tb_construct_bti(tb);
    tb_dut_reset(tb);

    tb_bti_write_write_req(tb, 0x0001, RAM_BASE + 0x100, 0xdeadbeef, 0x0f);
    bool got_wrsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_wrsp, "write response received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0001, 0, true), "BTI write ok");

    tb_bti_write_read_req(tb, 0x0002, RAM_BASE + 0x100);
    bool got_rrsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_rrsp, "read response received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0002, 0xdeadbeef, true),
              "BTI read: data=0xdeadbeef");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, bti_write_strobe)
{
    TEST_BEGIN("BTI Write with Strobe");

    tb_construct_bti(tb);
    tb_dut_reset(tb);

    tb_bti_write_write_req(tb, 0x0010, RAM_BASE + 0x200, 0xaabbccdd, 0x0f);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0010, 0, true), "full strobe: write ok");

    tb_bti_write_read_req(tb, 0x0011, RAM_BASE + 0x200);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0011, 0xaabbccdd, true),
              "full strobe: data=0xaabbccdd");

    tb_bti_write_write_req(tb, 0x0012, RAM_BASE + 0x300, 0xffffffff, 0x0f);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0012, 0, true), "init: write 0xffffffff");

    tb_bti_write_write_req(tb, 0x0013, RAM_BASE + 0x300, 0x12345678, 0x03);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0013, 0, true), "partial strobe: write 0x12345678");

    tb_bti_write_read_req(tb, 0x0014, RAM_BASE + 0x300);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0014, 0xffff5678, true),
              "partial strobe (0x03): only low 2 bytes changed");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, bti_size_and_boundary)
{
    TEST_BEGIN("BTI Size and Boundary");

    tb_construct_bti(tb);
    tb_dut_reset(tb);

    tb_bti_write_write_req(tb, 0x0020, RAM_BASE + 0x100, 0x44332211, 0xf);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0020, 0, true), "initialize test word");

    tb_bti_write_read_req_size(tb, 0x0021, RAM_BASE + 0x101, BTI_REQ_SIZE_B1);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0021, 0x22, true),
        "byte read is returned in low bits");

    tb_bti_write_read_req_size(tb, 0x0022, RAM_BASE + 0x101, BTI_REQ_SIZE_B2);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0022, 0x3322, true),
        "halfword read is returned in low bits");

    tb_bti_write_write_req_size(tb, 0x0023, RAM_BASE + 0x101, 0x0000aabb, 0xf,
        BTI_REQ_SIZE_B1);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0023, 0, true), "byte write accepted");
    tb_bti_write_read_req(tb, 0x0024, RAM_BASE + 0x100);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0024, 0x4433bb11, true),
        "byte write ignores strobes outside request size");

    tb_bti_write_write_req_size(tb, 0x0025, RAM_BASE + RAM_SIZE - 1, 0x5a, 0x1,
        BTI_REQ_SIZE_B1);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0025, 0, true), "last byte is writable");
    tb_bti_write_read_req_size(tb, 0x0026, RAM_BASE + RAM_SIZE - 1, BTI_REQ_SIZE_B1);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0026, 0x5a, true), "last byte is readable");
    tb_bti_write_read_req_size(tb, 0x0027, RAM_BASE + RAM_SIZE - 1, BTI_REQ_SIZE_B2);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0027, 0, false),
        "access crossing RAM boundary is rejected");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, axi_single_read)
{
    TEST_BEGIN("AXI Single Beat Read");

    tb_construct_axi(tb);
    tb_dut_reset(tb);

    tb_axi_write_aw(tb, 0x01, RAM_BASE + 0x40, 0, AXI4_AW_SIZE_B4, AXI4_AW_BURST_INCR);
    tb_axi_write_w(tb, 0xcafebabe, 0x0f, true);

    bool got_b = RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
    REQUIRE(got_b, "write B received");
    REQUIRE(tb_axi_check_and_pop_b(tb, 0x01, AXI4_B_RESP_OKAY), "write B: id=0x01 resp=0");

    tb_axi_write_ar(tb, 0x02, RAM_BASE + 0x40, 0, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);

    bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
    REQUIRE(got_r, "read R received");
    REQUIRE(tb_axi_check_and_pop_r(tb, 0x02, 0xcafebabe, AXI4_R_RESP_OKAY, true),
              "read R: id=0x02 data=0xcafebabe last=true");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, axi_burst_read_incr)
{
    TEST_BEGIN("AXI Burst Read (INCR, 4 beats)");

    tb_construct_axi(tb);
    tb_dut_reset(tb);

    for (u32 i = 0; i < 4; i++) {
        tb_axi_write_aw(tb, (u8)i, RAM_BASE + 0x80 + i * 4, 0, AXI4_AW_SIZE_B4, AXI4_AW_BURST_INCR);
        tb_axi_write_w(tb, 0xa0000000 + i, 0x0f, true);
        RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
        tb_axi_check_and_pop_b(tb, (u8)i, 0);
    }

    tb_axi_write_ar(tb, 0x10, RAM_BASE + 0x80, 3, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);

    for (u32 i = 0; i < 4; i++) {
        bool last = (i == 3);
        char msg[64];
        snprintf(msg, sizeof(msg), "beat %u: data=0x%08x last=%d",
                 i, 0xa0000000 + i, last);
        bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
        REQUIRE(got_r, msg);
        REQUIRE(tb_axi_check_and_pop_r(tb, 0x10, 0xa0000000 + i, 0, last), msg);
    }

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, axi_burst_write_incr)
{
    TEST_BEGIN("AXI Burst Write (INCR, 4 beats)");

    tb_construct_axi(tb);
    tb_dut_reset(tb);

    tb_axi_write_aw(tb, 0x20, RAM_BASE + 0x100, 3, AXI4_AW_SIZE_B4, AXI4_AW_BURST_INCR);

    u32 wr_data[] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    for (u32 i = 0; i < 4; i++) {
        tb_axi_write_w(tb, wr_data[i], 0x0f, (i == 3));
    }

    bool got_b = RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
    REQUIRE(got_b, "write B received");
    REQUIRE(tb_axi_check_and_pop_b(tb, 0x20, AXI4_B_RESP_OKAY), "write B: id=0x20 resp=0");

    for (u32 i = 0; i < 4; i++) {
        tb_axi_write_ar(tb, 0x30, RAM_BASE + 0x100 + i * 4, 0, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);
        bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
        char msg[64];
        snprintf(msg, sizeof(msg), "verify beat %u: data=0x%08x", i, wr_data[i]);
        REQUIRE(got_r, msg);
        REQUIRE(tb_axi_check_and_pop_r(tb, 0x30, wr_data[i], AXI4_R_RESP_OKAY, true), msg);
    }

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, axi_burst_fixed)
{
    TEST_BEGIN("AXI Fixed Burst (same address, 4 beats)");

    tb_construct_axi(tb);
    tb_dut_reset(tb);

    tb_axi_write_aw(tb, 0x40, RAM_BASE + 0x200, 3, AXI4_AW_SIZE_B4, AXI4_AW_BURST_FIXED);
    tb_axi_write_w(tb, 0xaaaaaaaa, 0x0f, true);
    RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
    tb_axi_check_and_pop_b(tb, 0x40, AXI4_B_RESP_OKAY);

    tb_axi_write_ar(tb, 0x41, RAM_BASE + 0x200, 3, AXI4_AR_SIZE_B4, AXI4_AR_BURST_FIXED);

    for (u32 i = 0; i < 4; i++) {
        bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
        char msg[48];
        snprintf(msg, sizeof(msg), "fixed beat %u: same addr, same data", i);
        REQUIRE(got_r, msg);
        REQUIRE(tb_axi_check_and_pop_r(tb, 0x41, 0xaaaaaaaa, AXI4_R_RESP_OKAY, (i == 3)), msg);
    }

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, axi_concurrent_rw)
{
    TEST_BEGIN("AXI Concurrent Read and Write");

    tb_construct_axi(tb);
    tb_dut_reset(tb);

    tb_axi_write_aw(tb, 0x50, RAM_BASE + 0x400, 0, AXI4_AW_SIZE_B4, AXI4_AW_BURST_INCR);
    tb_axi_write_w(tb, 0xabcd1234, 0x0f, true);

    tb_axi_write_ar(tb, 0x51, RAM_BASE + 0x500, 0, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);

    bool got_b = RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
    REQUIRE(got_b, "write B received");
    REQUIRE(tb_axi_check_and_pop_b(tb, 0x50, AXI4_B_RESP_OKAY), "write B: id=0x50");

    bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
    REQUIRE(got_r, "read R received (addr 0x500 uninitialized = 0)");
    REQUIRE(tb_axi_check_and_pop_r(tb, 0x51, 0, AXI4_R_RESP_OKAY, true),
              "read R: id=0x51 data=0 (uninitialized)");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(ram_tb_t, axi_write_strobe)
{
    TEST_BEGIN("AXI Write with Byte Strobes");

    tb_construct_axi(tb);
    tb_dut_reset(tb);

    tb_axi_write_aw(tb, 0x60, RAM_BASE + 0x600, 0, AXI4_AW_SIZE_B4, AXI4_AW_BURST_INCR);
    tb_axi_write_w(tb, 0xffffffff, 0x0f, true);
    RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
    tb_axi_check_and_pop_b(tb, 0x60, AXI4_B_RESP_OKAY);

    tb_axi_write_aw(tb, 0x61, RAM_BASE + 0x600, 0, AXI4_AW_SIZE_B4, AXI4_AW_BURST_INCR);
    tb_axi_write_w(tb, 0x12345678, 0x03, true);
    RUN_POLL_UNTIL(tb_cond_axi_b_ready, UT_TIMEOUT);
    tb_axi_check_and_pop_b(tb, 0x61, AXI4_B_RESP_OKAY);

    tb_axi_write_ar(tb, 0x62, RAM_BASE + 0x600, 0, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);
    RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
    REQUIRE(tb_axi_check_and_pop_r(tb, 0x62, 0xffff5678, AXI4_R_RESP_OKAY, true),
              "strobe 0x03: only low 2 bytes overwritten -> 0xffff5678");

    tb_free_dut(tb);
    TEST_END();
}

int main()
{
    ram_tb_t tb;
    tb_construct(&tb, "tb");

    TEST_RUN(bti_read);
    TEST_RUN(bti_write_strobe);
    TEST_RUN(bti_size_and_boundary);
    TEST_RUN(axi_single_read);
    TEST_RUN(axi_burst_read_incr);
    TEST_RUN(axi_burst_write_incr);
    TEST_RUN(axi_burst_fixed);
    TEST_RUN(axi_concurrent_rw);
    TEST_RUN(axi_write_strobe);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);
    return ret;
}
