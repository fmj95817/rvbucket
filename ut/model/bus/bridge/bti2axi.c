#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus/bridge.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct bti2axi_tb {
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

    bti2axi_t dut;

    u32 mock_rd_data;
    u8  mock_rd_resp;
    u8  mock_wr_resp;

    bool mock_saw_ar;
    u32 mock_ar_addr;
    u8 mock_ar_size;
    bool mock_saw_aw;
    u32 mock_aw_addr;
    u8 mock_aw_size;
    bool mock_saw_w;
    u32 mock_w_data;
    u8  mock_w_strb;

    ut_sbd_t sbd;
} bti2axi_tb_t;

static void tb_construct(bti2axi_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp_itf, 1);
    AXI4_AW_IF_CONSTRUCT(tb, axi4_aw_itf, 1);
    AXI4_W_IF_CONSTRUCT(tb, axi4_w_itf, 1);
    AXI4_B_IF_CONSTRUCT(tb, axi4_b_itf, 1);
    AXI4_AR_IF_CONSTRUCT(tb, axi4_ar_itf, 1);
    AXI4_R_IF_CONSTRUCT(tb, axi4_r_itf, 1);

    tb->dut.bti_req_slv = &tb->bti_req_itf;
    tb->dut.bti_rsp_mst = &tb->bti_rsp_itf;
    tb->dut.axi4_aw_mst = &tb->axi4_aw_itf;
    tb->dut.axi4_w_mst = &tb->axi4_w_itf;
    tb->dut.axi4_b_slv = &tb->axi4_b_itf;
    tb->dut.axi4_ar_mst = &tb->axi4_ar_itf;
    tb->dut.axi4_r_slv = &tb->axi4_r_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    bti2axi_construct(&tb->dut, tb->mod.hier_name, "u_dut");

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(bti2axi_tb_t *tb)
{
    bti2axi_reset(&tb->dut);
    dbg_vcd_reset();
    tb->mock_rd_data = 0;
    tb->mock_rd_resp = AXI4_R_RESP_OKAY;
    tb->mock_wr_resp = AXI4_B_RESP_OKAY;
    tb->mock_saw_ar = false;
    tb->mock_ar_addr = 0;
    tb->mock_ar_size = 0;
    tb->mock_saw_aw = false;
    tb->mock_aw_addr = 0;
    tb->mock_aw_size = 0;
    tb->mock_saw_w = false;
    tb->mock_w_data = 0;
    tb->mock_w_strb = 0;
}

static void tb_mock_axi4_slave(bti2axi_tb_t *tb)
{
    if (!itf_fifo_empty(&tb->axi4_ar_itf) && !itf_fifo_full(&tb->axi4_r_itf)) {
        axi4_ar_if_t ar;
        itf_read(&tb->axi4_ar_itf, &ar);
        tb->mock_saw_ar = true;
        tb->mock_ar_addr = ar.addr;
        tb->mock_ar_size = ar.size;

        axi4_r_if_t r = {
            .id = 0,
            .data = tb->mock_rd_data,
            .resp = tb->mock_rd_resp,
            .last = true
        };
        itf_write(&tb->axi4_r_itf, &r);
    }

    if (!itf_fifo_empty(&tb->axi4_aw_itf) && !itf_fifo_empty(&tb->axi4_w_itf) &&
        !itf_fifo_full(&tb->axi4_b_itf)) {
        axi4_aw_if_t aw;
        itf_read(&tb->axi4_aw_itf, &aw);
        tb->mock_saw_aw = true;
        tb->mock_aw_addr = aw.addr;
        tb->mock_aw_size = aw.size;

        axi4_w_if_t w;
        itf_read(&tb->axi4_w_itf, &w);
        tb->mock_saw_w = true;
        tb->mock_w_data = w.data;
        tb->mock_w_strb = w.strb;

        axi4_b_if_t b = {
            .id = 0,
            .resp = tb->mock_wr_resp
        };
        itf_write(&tb->axi4_b_itf, &b);
    }
}

static void tb_clock(bti2axi_tb_t *tb)
{
    bti2axi_clock(&tb->dut);
    tb_mock_axi4_slave(tb);

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

static void tb_free(bti2axi_tb_t *tb)
{
    bti2axi_free(&tb->dut);

    itf_free(&tb->bti_req_itf);
    itf_free(&tb->bti_rsp_itf);
    itf_free(&tb->axi4_aw_itf);
    itf_free(&tb->axi4_w_itf);
    itf_free(&tb->axi4_b_itf);
    itf_free(&tb->axi4_ar_itf);
    itf_free(&tb->axi4_r_itf);
}

static void tb_bti_write_read_req(bti2axi_tb_t *tb, u16 trans_id, u32 addr,
    bti_req_size_t size)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_READ,
        .addr = addr,
        .size = size,
        .data = 0,
        .strobe = 0
    };
    itf_write(&tb->bti_req_itf, &req);
}

static void tb_bti_write_write_req(bti2axi_tb_t *tb, u16 trans_id, u32 addr,
    u32 data, u8 strobe, bti_req_size_t size)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_WRITE,
        .addr = addr,
        .size = size,
        .data = data,
        .strobe = strobe
    };
    itf_write(&tb->bti_req_itf, &req);
}

static void tb_bti_write_cbo_req(bti2axi_tb_t *tb, u16 trans_id, u32 addr,
    bti_req_cmd_t cmd)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = cmd,
        .addr = addr,
        .size = BTI_REQ_SIZE_B4,
        .data = 0,
        .strobe = 0
    };
    itf_write(&tb->bti_req_itf, &req);
}

static bool tb_cond_bti_rsp_ready(bti2axi_tb_t *tb)
{
    return !itf_fifo_empty(&tb->bti_rsp_itf);
}

static bool tb_cond_mock_saw_ar(bti2axi_tb_t *tb)
{
    return tb->mock_saw_ar;
}

static bool tb_cond_mock_saw_aw(bti2axi_tb_t *tb)
{
    return tb->mock_saw_aw;
}

static bool tb_bti_check_and_pop_rsp(bti2axi_tb_t *tb, u16 expected_trans_id,
                                      u32 expected_data, bool expected_ok)
{
    if (itf_fifo_empty(&tb->bti_rsp_itf)) {
        return false;
    }
    bti_rsp_if_t rsp;
    itf_read(&tb->bti_rsp_itf, &rsp);
    return (rsp.trans_id == expected_trans_id) &&
           (rsp.data == expected_data) &&
           (rsp.ok == expected_ok);
}

TEST_CASE(bti2axi_tb_t, read)
{
    TEST_BEGIN("BTI Read");

    tb->mock_rd_data = 0xdeadbeef;
    tb->mock_saw_ar = false;

    tb_bti_write_read_req(tb, 0x0001, 0x12345678, BTI_REQ_SIZE_B1);

    bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_saw_ar, UT_TIMEOUT);
    REQUIRE(got_ar, "mock saw AR on AXI4 bus");
    REQUIRE(tb->mock_ar_addr == 0x12345678, "AR addr = 0x12345678");
    REQUIRE(tb->mock_ar_size == AXI4_AR_SIZE_B1, "AR size preserves BTI byte access");

    bool got_rsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_rsp, "BTI rsp received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0001, 0xdeadbeef, true),
              "BTI rsp: trans_id=0x0001 data=0xdeadbeef ok=true");
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, read_slverr)
{
    TEST_BEGIN("BTI Read with SLVERR");

    tb->mock_rd_data = 0;
    tb->mock_rd_resp = 2;
    tb->mock_saw_ar = false;

    tb_bti_write_read_req(tb, 0x0002, 0xa0000000, BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_mock_saw_ar, UT_TIMEOUT);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0002, 0, false),
              "BTI rsp: ok=false on SLVERR");
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, write)
{
    TEST_BEGIN("BTI Write");

    tb->mock_saw_aw = false;
    tb->mock_saw_w = false;

    tb_bti_write_write_req(tb, 0x0003, 0x20001000, 0xcafebabe, 0x03,
        BTI_REQ_SIZE_B2);

    bool got_aw = RUN_POLL_UNTIL(tb_cond_mock_saw_aw, UT_TIMEOUT);
    REQUIRE(got_aw, "mock saw AW on AXI4 bus");
    REQUIRE(tb->mock_aw_addr == 0x20001000, "AW addr = 0x20001000");
    REQUIRE(tb->mock_aw_size == AXI4_AW_SIZE_B2, "AW size preserves BTI halfword access");
    REQUIRE(tb->mock_saw_w, "mock also saw W in same cycle");
    REQUIRE(tb->mock_w_data == 0xcafebabe, "W data = 0xcafebabe");
    REQUIRE(tb->mock_w_strb == 0x03, "W strb = 0x03");

    bool got_rsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_rsp, "BTI rsp received");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0003, 0, true),
              "BTI rsp: trans_id=0x0003 data=0 ok=true");
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, write_slverr)
{
    TEST_BEGIN("BTI Write with SLVERR");

    tb->mock_saw_aw = false;
    tb->mock_wr_resp = 2;

    tb_bti_write_write_req(tb, 0x0030, 0x50000000, 0x11111111, 0x0f,
        BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_mock_saw_aw, UT_TIMEOUT);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0030, 0, false),
              "BTI write rsp: ok=false on SLVERR");
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, strobe_mask)
{
    TEST_BEGIN("Write Strobe Masking");

    tb->mock_wr_resp = 0;
    tb->mock_saw_aw = false;
    tb->mock_saw_w = false;

    tb_bti_write_write_req(tb, 0x0020, 0x40000000, 0x12345678, 0xff,
        BTI_REQ_SIZE_B4);
    RUN_POLL_UNTIL(tb_cond_mock_saw_aw, UT_TIMEOUT);
    REQUIRE(tb->mock_w_strb == 0x0f, "BTI strobe 0xff masked to AXI4 strb 0x0f");

    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    tb_bti_check_and_pop_rsp(tb, 0x0020, 0, true);
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, back_to_back)
{
    TEST_BEGIN("Back-to-Back Reads");

    tb->mock_rd_data = 0xaaaa5555;
    tb->mock_rd_resp = 0;
    tb->mock_saw_ar = false;

    tb_bti_write_read_req(tb, 0x0010, 0x10000000, BTI_REQ_SIZE_B4);

    RUN_POLL_UNTIL(tb_cond_mock_saw_ar, UT_TIMEOUT);
    REQUIRE(tb->mock_ar_addr == 0x10000000, "1st read: AR addr=0x10000000");

    tb_bti_write_read_req(tb, 0x0011, 0x20000000, BTI_REQ_SIZE_B4);
    REQUIRE(!itf_fifo_empty(&tb->bti_req_itf),
              "2nd read: BTI req queued in FIFO (DUT busy with 1st outstanding)");

    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(!itf_fifo_empty(&tb->bti_req_itf),
              "2nd read: still stalled after 1st rsp (needs another cycle)");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0010, 0xaaaa5555, true),
              "1st read: rsp received");

    tb->mock_rd_data = 0xbbbb6666;
    tb->mock_saw_ar = false;

    RUN_POLL_UNTIL(tb_cond_mock_saw_ar, UT_TIMEOUT);
    REQUIRE(tb->mock_ar_addr == 0x20000000, "2nd read: AR addr=0x20000000");

    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0011, 0xbbbb6666, true),
              "2nd read: rsp received");
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, sequential_reads)
{
    TEST_BEGIN("Sequential Reads (3 transactions)");

    for (u32 i = 0; i < 3; i++) {
        u16 tid = 0x100 + (u16)i;
        u32 addr = 0x30000000 + i * 0x100;
        u32 data = 0xa0000000 + i;

        tb->mock_rd_data = data;
        tb->mock_rd_resp = 0;
        tb->mock_saw_ar = false;

        tb_bti_write_read_req(tb, tid, addr, BTI_REQ_SIZE_B4);

        bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_saw_ar, UT_TIMEOUT);
        if (!got_ar) {
            REQUIRE(false, "seq read: timeout waiting for AR");
            return;
        }

        bool got_rsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
        if (!got_rsp) {
            REQUIRE(false, "seq read: timeout waiting for BTI rsp");
            return;
        }

        char msg[128];
        snprintf(msg, sizeof(msg), "seq read[%u]: trans_id=0x%04x addr=0x%08x data=0x%08x ok=true",
                 i, tid, addr, data);
        REQUIRE(tb_bti_check_and_pop_rsp(tb, tid, data, true), msg);
    }

    TEST_END();
}

TEST_CASE(bti2axi_tb_t, unsupported_cmd_does_not_touch_axi)
{
    TEST_BEGIN("Unsupported BTI Command Does Not Touch AXI");

    tb->mock_saw_ar = false;
    tb->mock_saw_aw = false;
    tb->mock_saw_w = false;

    tb_bti_write_cbo_req(tb, 0x00c0, 0x40000000, BTI_REQ_CMD_CBO_CLEAN);

    bool got_rsp = RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(got_rsp, "unsupported command returns a BTI response");
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x00c0, 0, false),
        "unsupported command response fails");
    REQUIRE(!tb->mock_saw_ar && !tb->mock_saw_aw && !tb->mock_saw_w,
        "unsupported command does not issue AXI read or write");

    TEST_END();
}

int main()
{
    bti2axi_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(read);
    TEST_RUN(read_slverr);
    TEST_RUN(write);
    TEST_RUN(write_slverr);
    TEST_RUN(strobe_mask);
    TEST_RUN(back_to_back);
    TEST_RUN(sequential_reads);
    TEST_RUN(unsupported_cmd_does_not_touch_axi);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);

    return ret;
}
