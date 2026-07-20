#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus/bridge.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define BTI2AXI_TB_FIFO_DEPTH 16
#define BTI2AXI_TB_STG_FIFO_DEPTH 8
#define BTI2AXI_TB_OST_DEPTH 8
#define BTI2AXI_TB_MAX_SEEN 32

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
    bool mock_auto_rsp;

    bool mock_saw_ar;
    u32 mock_ar_addr;
    u8 mock_ar_size;
    u32 mock_ar_count;
    u32 mock_ar_addrs[BTI2AXI_TB_MAX_SEEN];
    u8  mock_ar_ids[BTI2AXI_TB_MAX_SEEN];
    bool mock_saw_aw;
    u32 mock_aw_addr;
    u8 mock_aw_size;
    u32 mock_aw_count;
    u32 mock_aw_addrs[BTI2AXI_TB_MAX_SEEN];
    u8  mock_aw_ids[BTI2AXI_TB_MAX_SEEN];
    bool mock_saw_w;
    u32 mock_w_data;
    u8  mock_w_strb;
    u32 mock_w_count;
    u32 mock_w_data_list[BTI2AXI_TB_MAX_SEEN];

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

    BTI_REQ_IF_CONSTRUCT(tb, bti_req_itf, BTI2AXI_TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp_itf, BTI2AXI_TB_FIFO_DEPTH);
    AXI4_AW_IF_CONSTRUCT(tb, axi4_aw_itf, BTI2AXI_TB_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, axi4_w_itf, BTI2AXI_TB_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, axi4_b_itf, BTI2AXI_TB_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, axi4_ar_itf, BTI2AXI_TB_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, axi4_r_itf, BTI2AXI_TB_FIFO_DEPTH);

    tb->dut.bti_req_slv = &tb->bti_req_itf;
    tb->dut.bti_rsp_mst = &tb->bti_rsp_itf;
    tb->dut.axi4_aw_mst = &tb->axi4_aw_itf;
    tb->dut.axi4_w_mst = &tb->axi4_w_itf;
    tb->dut.axi4_b_slv = &tb->axi4_b_itf;
    tb->dut.axi4_ar_mst = &tb->axi4_ar_itf;
    tb->dut.axi4_r_slv = &tb->axi4_r_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    bti2axi_conf_t conf = {
        .stg_fifo_depth = BTI2AXI_TB_STG_FIFO_DEPTH,
        .ost_depth = BTI2AXI_TB_OST_DEPTH
    };
    bti2axi_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(bti2axi_tb_t *tb)
{
    bti2axi_reset(&tb->dut);
    itf_reset(&tb->bti_req_itf);
    itf_reset(&tb->bti_rsp_itf);
    itf_reset(&tb->axi4_aw_itf);
    itf_reset(&tb->axi4_w_itf);
    itf_reset(&tb->axi4_b_itf);
    itf_reset(&tb->axi4_ar_itf);
    itf_reset(&tb->axi4_r_itf);
    dbg_vcd_reset();
    tb->mock_rd_data = 0;
    tb->mock_rd_resp = AXI4_R_RESP_OKAY;
    tb->mock_wr_resp = AXI4_B_RESP_OKAY;
    tb->mock_auto_rsp = true;
    tb->mock_saw_ar = false;
    tb->mock_ar_addr = 0;
    tb->mock_ar_size = 0;
    tb->mock_ar_count = 0;
    tb->mock_saw_aw = false;
    tb->mock_aw_addr = 0;
    tb->mock_aw_size = 0;
    tb->mock_aw_count = 0;
    tb->mock_saw_w = false;
    tb->mock_w_data = 0;
    tb->mock_w_strb = 0;
    tb->mock_w_count = 0;
}

static void tb_mock_axi4_slave(bti2axi_tb_t *tb)
{
    if (!itf_fifo_empty(&tb->axi4_ar_itf) && !itf_fifo_full(&tb->axi4_r_itf)) {
        axi4_ar_if_t ar;
        itf_read(&tb->axi4_ar_itf, &ar);
        tb->mock_saw_ar = true;
        tb->mock_ar_addr = ar.addr;
        tb->mock_ar_size = ar.size;
        if (tb->mock_ar_count < BTI2AXI_TB_MAX_SEEN) {
            tb->mock_ar_addrs[tb->mock_ar_count] = ar.addr;
            tb->mock_ar_ids[tb->mock_ar_count] = ar.id;
        }
        tb->mock_ar_count++;

        if (tb->mock_auto_rsp) {
            axi4_r_if_t r = {
                .id = ar.id,
                .data = tb->mock_rd_data,
                .resp = tb->mock_rd_resp,
                .last = true
            };
            itf_write(&tb->axi4_r_itf, &r);
        }
    }

    if (!itf_fifo_empty(&tb->axi4_aw_itf) && !itf_fifo_empty(&tb->axi4_w_itf) &&
        !itf_fifo_full(&tb->axi4_b_itf)) {
        axi4_aw_if_t aw;
        itf_read(&tb->axi4_aw_itf, &aw);
        tb->mock_saw_aw = true;
        tb->mock_aw_addr = aw.addr;
        tb->mock_aw_size = aw.size;
        if (tb->mock_aw_count < BTI2AXI_TB_MAX_SEEN) {
            tb->mock_aw_addrs[tb->mock_aw_count] = aw.addr;
            tb->mock_aw_ids[tb->mock_aw_count] = aw.id;
        }
        tb->mock_aw_count++;

        axi4_w_if_t w;
        itf_read(&tb->axi4_w_itf, &w);
        tb->mock_saw_w = true;
        tb->mock_w_data = w.data;
        tb->mock_w_strb = w.strb;
        if (tb->mock_w_count < BTI2AXI_TB_MAX_SEEN) {
            tb->mock_w_data_list[tb->mock_w_count] = w.data;
        }
        tb->mock_w_count++;

        if (tb->mock_auto_rsp) {
            axi4_b_if_t b = {
                .id = aw.id,
                .resp = tb->mock_wr_resp
            };
            itf_write(&tb->axi4_b_itf, &b);
        }
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

static u32 tb_fifo_count(itf_t *itf)
{
    return itf->ctx.fifo.pkt_num;
}

static void tb_axi_write_r(bti2axi_tb_t *tb, u8 id, u32 data, axi4_r_resp_t resp)
{
    axi4_r_if_t r = {
        .id = id,
        .data = data,
        .resp = resp,
        .last = true
    };
    itf_write(&tb->axi4_r_itf, &r);
}

static void tb_axi_write_b(bti2axi_tb_t *tb, u8 id, axi4_b_resp_t resp)
{
    axi4_b_if_t b = {
        .id = id,
        .resp = resp
    };
    itf_write(&tb->axi4_b_itf, &b);
}

TEST_CASE(bti2axi_tb_t, read)
{
    TEST_BEGIN("BTI Read");
    tb_reset(tb);

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
    tb_reset(tb);

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
    tb_reset(tb);

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
    tb_reset(tb);

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
    tb_reset(tb);

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
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    tb_bti_write_read_req(tb, 0x0010, 0x10000000, BTI_REQ_SIZE_B4);
    tb_bti_write_read_req(tb, 0x0011, 0x20000000, BTI_REQ_SIZE_B4);

    RUN_CYCLES(2);
    REQUIRE(tb->mock_ar_count == 2, "two ARs issued before any R response");
    REQUIRE(tb->mock_ar_addrs[0] == 0x10000000, "1st read: AR addr=0x10000000");
    REQUIRE(tb->mock_ar_addrs[1] == 0x20000000, "2nd read: AR addr=0x20000000");
    REQUIRE(ostk_count(&tb->dut.rd_ost) == 2, "two read requests are outstanding");

    tb_axi_write_r(tb, BTI2AXI_AXI_ID, 0xaaaa5555, AXI4_R_RESP_OKAY);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0010, 0xaaaa5555, true),
              "1st read: rsp received");

    tb_axi_write_r(tb, BTI2AXI_AXI_ID, 0xbbbb6666, AXI4_R_RESP_OKAY);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0011, 0xbbbb6666, true),
              "2nd read: rsp received");
    TEST_END();
}

TEST_CASE(bti2axi_tb_t, back_to_back_writes)
{
    TEST_BEGIN("Back-to-Back Writes");
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    tb_bti_write_write_req(tb, 0x0040, 0x10001000, 0xaaaa0001, 0x0f,
        BTI_REQ_SIZE_B4);
    tb_bti_write_write_req(tb, 0x0041, 0x10001004, 0xbbbb0002, 0x0f,
        BTI_REQ_SIZE_B4);

    RUN_CYCLES(2);
    REQUIRE(tb->mock_aw_count == 2, "two AWs issued before any B response");
    REQUIRE(tb->mock_w_count == 2, "two Ws issued before any B response");
    REQUIRE(tb->mock_aw_addrs[0] == 0x10001000, "1st write: AW addr preserved");
    REQUIRE(tb->mock_aw_addrs[1] == 0x10001004, "2nd write: AW addr preserved");
    REQUIRE(tb->mock_w_data_list[0] == 0xaaaa0001, "1st write: W data preserved");
    REQUIRE(tb->mock_w_data_list[1] == 0xbbbb0002, "2nd write: W data preserved");
    REQUIRE(ostk_count(&tb->dut.wr_ost) == 2, "two write requests are outstanding");

    tb_axi_write_b(tb, BTI2AXI_AXI_ID, AXI4_B_RESP_OKAY);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0040, 0, true),
            "1st write: BTI rsp received");

    tb_axi_write_b(tb, BTI2AXI_AXI_ID, AXI4_B_RESP_OKAY);
    RUN_POLL_UNTIL(tb_cond_bti_rsp_ready, UT_TIMEOUT);
    REQUIRE(tb_bti_check_and_pop_rsp(tb, 0x0041, 0, true),
            "2nd write: BTI rsp received");

    TEST_END();
}

TEST_CASE(bti2axi_tb_t, read_table_full_backpressures)
{
    TEST_BEGIN("Read Table Full Backpressures");
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    for (u32 i = 0; i < BTI2AXI_TB_OST_DEPTH + 1u; i++) {
        tb_bti_write_read_req(tb, (u16)(0x200u + i),
            0x30000000 + i * 4u, BTI_REQ_SIZE_B4);
    }

    RUN_CYCLES(BTI2AXI_TB_OST_DEPTH + 2u);
    REQUIRE(tb->mock_ar_count == BTI2AXI_TB_OST_DEPTH,
            "only table-depth ARs are issued");
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == 0,
            "held BTI request is captured into ingress FIFO");
    REQUIRE(fifo_count(&tb->dut.bti_req_fifo) == 1,
            "one BTI request waits in bridge ingress while table is full");
    REQUIRE(ostk_count(&tb->dut.rd_ost) == BTI2AXI_TB_OST_DEPTH,
            "read outstanding table is full");

    tb_axi_write_r(tb, BTI2AXI_AXI_ID, 0x12345678, AXI4_R_RESP_OKAY);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->bti_rsp_itf) == 1,
            "first response is produced");
    REQUIRE(fifo_count(&tb->dut.bti_req_fifo) == 1,
            "ingress request waits until free clock");
    REQUIRE(tb->mock_ar_count == BTI2AXI_TB_OST_DEPTH,
            "no same-cycle AR is issued from freed slot");

    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == 0,
            "BTI request interface remains drained after free clock");
    REQUIRE(fifo_count(&tb->dut.bti_req_fifo) == 0,
            "ingress request is issued after free clock");
    REQUIRE(tb->mock_ar_count == BTI2AXI_TB_OST_DEPTH + 1u,
            "previously blocked AR is issued");

    TEST_END();
}

TEST_CASE(bti2axi_tb_t, sequential_reads)
{
    TEST_BEGIN("Sequential Reads (3 transactions)");
    tb_reset(tb);
    tb->mock_auto_rsp = false;

    for (u32 i = 0; i < 3; i++) {
        u16 tid = 0x100 + (u16)i;
        u32 addr = 0x30000000 + i * 0x100;
        u32 data = 0xa0000000 + i;

        tb->mock_saw_ar = false;

        tb_bti_write_read_req(tb, tid, addr, BTI_REQ_SIZE_B4);

        bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_saw_ar, UT_TIMEOUT);
        if (!got_ar) {
            REQUIRE(false, "seq read: timeout waiting for AR");
            return;
        }
        REQUIRE(tb->mock_ar_addr == addr, "seq read: AR address matches request");

        tb_axi_write_r(tb, BTI2AXI_AXI_ID, data, AXI4_R_RESP_OKAY);

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
    TEST_RUN(back_to_back_writes);
    TEST_RUN(read_table_full_backpressures);
    TEST_RUN(sequential_reads);
    TEST_RUN(unsupported_cmd_does_not_touch_axi);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);

    return ret;
}
