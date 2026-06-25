#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem/rom.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define ROM_SIZE 0x1000
#define ROM_BASE 0x80000000

static u8 g_init_data[256];

typedef struct rom_tb {
    u64 *cycle;
    u64 cycle_val;

    itf_t bti_req_itf;
    itf_t bti_rsp_itf;

    itf_t axi4_aw_itf;
    itf_t axi4_w_itf;
    itf_t axi4_b_itf;
    itf_t axi4_ar_itf;
    itf_t axi4_r_itf;

    rom_t dut;

    ut_sbd_t sbd;
} rom_tb_t;

static void tb_construct(rom_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;

    BTI_REQ_IF_CONSTRUCT(tb, bti_req_itf, 4);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp_itf, 4);

    AXI4_AW_IF_CONSTRUCT(tb, axi4_aw_itf, 4);
    AXI4_W_IF_CONSTRUCT(tb, axi4_w_itf, 4);
    AXI4_B_IF_CONSTRUCT(tb, axi4_b_itf, 4);
    AXI4_AR_IF_CONSTRUCT(tb, axi4_ar_itf, 4);
    AXI4_R_IF_CONSTRUCT(tb, axi4_r_itf, 4);

    ut_sbd_init(&tb->sbd);
}

static void tb_construct_bti(rom_tb_t *tb, u8 init_val)
{
    memset(g_init_data, init_val, sizeof(g_init_data));
    tb->dut.bti_req_slv = &tb->bti_req_itf;
    tb->dut.bti_rsp_mst = &tb->bti_rsp_itf;
    rom_construct(&tb->dut, "u_rom", ROM_MODE_BTI, ROM_SIZE,
                  g_init_data, sizeof(g_init_data), ROM_BASE);
}

static void tb_construct_axi(rom_tb_t *tb, u8 init_val)
{
    memset(g_init_data, init_val, sizeof(g_init_data));
    tb->dut.axi4_aw_slv = &tb->axi4_aw_itf;
    tb->dut.axi4_w_slv = &tb->axi4_w_itf;
    tb->dut.axi4_b_mst = &tb->axi4_b_itf;
    tb->dut.axi4_ar_slv = &tb->axi4_ar_itf;
    tb->dut.axi4_r_mst = &tb->axi4_r_itf;
    rom_construct(&tb->dut, "u_rom", ROM_MODE_AXI, ROM_SIZE,
                  g_init_data, sizeof(g_init_data), ROM_BASE);
}

static void tb_dut_reset(rom_tb_t *tb)
{
    rom_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_clock(rom_tb_t *tb)
{
    rom_clock(&tb->dut);

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

static void tb_free_dut(rom_tb_t *tb)
{
    rom_free(&tb->dut);
}

static void tb_free(rom_tb_t *tb)
{
    itf_free(&tb->bti_req_itf);
    itf_free(&tb->bti_rsp_itf);
    itf_free(&tb->axi4_aw_itf);
    itf_free(&tb->axi4_w_itf);
    itf_free(&tb->axi4_b_itf);
    itf_free(&tb->axi4_ar_itf);
    itf_free(&tb->axi4_r_itf);
}

static void tb_bti_write_read_req(rom_tb_t *tb, u16 trans_id, u32 addr)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_READ,
        .addr = addr,
        .data = 0,
        .strobe = 0
    };
    itf_write(&tb->bti_req_itf, &req);
}

static void tb_bti_write_write_req(rom_tb_t *tb, u16 trans_id, u32 addr,
                                    u32 data, u8 strobe)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_WRITE,
        .addr = addr,
        .data = data,
        .strobe = strobe
    };
    itf_write(&tb->bti_req_itf, &req);
}

static bool tb_cond_bti_rsp_ready(rom_tb_t *tb)
{
    return !itf_fifo_empty(&tb->bti_rsp_itf);
}

static bool tb_bti_check_and_pop_rsp(rom_tb_t *tb, u16 expected_trans_id,
                                      u32 expected_data, bool expected_ok)
{
    if (itf_fifo_empty(&tb->bti_rsp_itf)) return false;
    bti_rsp_if_t rsp;
    itf_read(&tb->bti_rsp_itf, &rsp);
    return (rsp.trans_id == expected_trans_id) &&
           (rsp.data == expected_data) &&
           (rsp.ok == expected_ok);
}

static void tb_axi_write_ar(rom_tb_t *tb, u8 id, u32 addr, u8 len,
                             axi4_ar_size_t size, axi4_ar_burst_t burst)
{
    axi4_ar_if_t ar = {
        .id = id, .addr = addr, .len = len, .size = size, .burst = burst,
        .lock = false, .cache = 0, .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->axi4_ar_itf, &ar);
}

static bool tb_cond_axi_r_ready(rom_tb_t *tb)
{
    return !itf_fifo_empty(&tb->axi4_r_itf);
}

static bool tb_axi_check_and_pop_r(rom_tb_t *tb, u8 expected_id, u32 expected_data,
                                    axi4_r_resp_t expected_resp, bool expected_last)
{
    if (itf_fifo_empty(&tb->axi4_r_itf)) return false;
    axi4_r_if_t r;
    itf_read(&tb->axi4_r_itf, &r);
    return (r.id == expected_id) && (r.data == expected_data) &&
           (r.resp == expected_resp) && (r.last == expected_last);
}

TEST_CASE(rom_tb_t, bti_read)
{
    TEST_BEGIN("BTI Read from initialized ROM");

    tb_construct_bti(tb, 0xcd);
    tb_dut_reset(tb);

    tb_bti_write_read_req(tb, 0x0001, ROM_BASE + 0x40);
    bool got_rsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_rsp, "BTI read response received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0001, 0xcdcdcdcd, true),
              "BTI read: data=0xcdcdcdcd (init value)");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(rom_tb_t, bti_write_rejected)
{
    TEST_BEGIN("BTI Write Rejected (read-only)");

    tb_construct_bti(tb, 0xab);
    tb_dut_reset(tb);

    tb_bti_write_write_req(tb, 0x0010, ROM_BASE + 0x80, 0x12345678, 0x0f);
    bool got_wrsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_wrsp, "write response received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0010, 0, false),
              "BTI write: ok=false (ROM is read-only)");

    tb_bti_write_read_req(tb, 0x0011, ROM_BASE + 0x80);
    bool got_rrsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_rrsp, "read back response received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0011, 0xabababab, true),
              "BTI read: data unchanged (write was rejected)");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(rom_tb_t, axi_read)
{
    TEST_BEGIN("AXI Single Beat Read");

    tb_construct_axi(tb, 0x5a);
    tb_dut_reset(tb);

    tb_axi_write_ar(tb, 0x01, ROM_BASE + 0x20, 0, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);

    bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
    REQUIRE(got_r, "AXI R received");
    REQUIRE(tb_axi_check_and_pop_r(tb, 0x01, 0x5a5a5a5a, AXI4_R_RESP_OKAY, true),
              "AXI read: data=0x5a5a5a5a resp=OKAY");

    tb_free_dut(tb);
    TEST_END();
}

TEST_CASE(rom_tb_t, axi_burst_read)
{
    TEST_BEGIN("AXI Burst Read (INCR, 4 beats)");

    tb_construct_axi(tb, 0x3c);
    tb_dut_reset(tb);

    tb_axi_write_ar(tb, 0x10, ROM_BASE + 0x80, 3, AXI4_AR_SIZE_B4, AXI4_AR_BURST_INCR);

    for (u32 i = 0; i < 4; i++) {
        bool last = (i == 3);
        char msg[48];
        snprintf(msg, sizeof(msg), "beat %u: data=0x3c3c3c3c last=%d", i, last);
        bool got_r = RUN_POLL_UNTIL(tb_cond_axi_r_ready, UT_TIMEOUT);
        REQUIRE(got_r, msg);
        REQUIRE(tb_axi_check_and_pop_r(tb, 0x10, 0x3c3c3c3c, AXI4_R_RESP_OKAY, last), msg);
    }

    tb_free_dut(tb);
    TEST_END();
}

int main()
{
    rom_tb_t tb;
    tb_construct(&tb, "tb");

    TEST_RUN(bti_read);
    TEST_RUN(bti_write_rejected);
    TEST_RUN(axi_read);
    TEST_RUN(axi_burst_read);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);
    return ret;
}
