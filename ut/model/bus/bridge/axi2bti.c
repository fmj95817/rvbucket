#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus/bridge.h"
#include "utils.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define AXI2BTI_TB_FIFO_DEPTH 16
#define AXI2BTI_TB_STG_FIFO_DEPTH 8
#define AXI2BTI_TB_OST_DEPTH 8

typedef struct axi2bti_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t axi4_ar_itf;
    itf_t axi4_r_itf;
    itf_t axi4_aw_itf;
    itf_t axi4_w_itf;
    itf_t axi4_b_itf;
    itf_t bti_req_itf;
    itf_t bti_rsp_itf;

    axi2bti_t dut;

    u32 mock_rd_data_off;
    u32 mock_err_on_req;
    u32 mock_req_count;
    u32 mock_last_addr;
    u32 mock_last_data;
    bool mock_last_write;
    bti_req_size_t mock_last_size;
    bool mock_auto_rsp;

    ut_sbd_t sbd;
} axi2bti_tb_t;

static void tb_construct(axi2bti_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    AXI4_AR_IF_CONSTRUCT(tb, axi4_ar_itf, AXI2BTI_TB_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, axi4_r_itf, AXI2BTI_TB_FIFO_DEPTH);
    AXI4_AW_IF_CONSTRUCT(tb, axi4_aw_itf, AXI2BTI_TB_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, axi4_w_itf, AXI2BTI_TB_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, axi4_b_itf, AXI2BTI_TB_FIFO_DEPTH);
    BTI_REQ_IF_CONSTRUCT(tb, bti_req_itf, AXI2BTI_TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp_itf, AXI2BTI_TB_FIFO_DEPTH);

    tb->dut.axi4_ar_slv = &tb->axi4_ar_itf;
    tb->dut.axi4_r_mst = &tb->axi4_r_itf;
    tb->dut.axi4_aw_slv = &tb->axi4_aw_itf;
    tb->dut.axi4_w_slv = &tb->axi4_w_itf;
    tb->dut.axi4_b_mst = &tb->axi4_b_itf;
    tb->dut.bti_req_mst = &tb->bti_req_itf;
    tb->dut.bti_rsp_slv = &tb->bti_rsp_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    axi2bti_conf_t conf = {
        .stg_fifo_depth = AXI2BTI_TB_STG_FIFO_DEPTH,
        .ost_depth = AXI2BTI_TB_OST_DEPTH
    };
    axi2bti_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(axi2bti_tb_t *tb)
{
    axi2bti_reset(&tb->dut);
    itf_reset(&tb->axi4_ar_itf);
    itf_reset(&tb->axi4_r_itf);
    itf_reset(&tb->axi4_aw_itf);
    itf_reset(&tb->axi4_w_itf);
    itf_reset(&tb->axi4_b_itf);
    itf_reset(&tb->bti_req_itf);
    itf_reset(&tb->bti_rsp_itf);
    dbg_vcd_reset();

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;
    tb->mock_last_addr = 0;
    tb->mock_last_data = 0;
    tb->mock_last_write = false;
    tb->mock_last_size = BTI_REQ_SIZE_B1;
    tb->mock_auto_rsp = true;
}

static void tb_drain_fifos(axi2bti_tb_t *tb)
{
    itf_fifo_pop_all(&tb->bti_req_itf);
    itf_fifo_pop_all(&tb->bti_rsp_itf);
    itf_fifo_pop_all(&tb->axi4_ar_itf);
    itf_fifo_pop_all(&tb->axi4_r_itf);
    itf_fifo_pop_all(&tb->axi4_aw_itf);
    itf_fifo_pop_all(&tb->axi4_w_itf);
    itf_fifo_pop_all(&tb->axi4_b_itf);
}

static void tb_mock_bti_slave(axi2bti_tb_t *tb)
{
    if (!tb->mock_auto_rsp) {
        return;
    }

    if (itf_fifo_empty(&tb->bti_req_itf) || itf_fifo_full(&tb->bti_rsp_itf)) {
        return;
    }

    bti_req_if_t req;
    itf_read(&tb->bti_req_itf, &req);

    tb->mock_req_count++;
    tb->mock_last_addr = req.addr;
    tb->mock_last_data = req.data;
    tb->mock_last_write = (req.cmd == BTI_REQ_CMD_WRITE);
    tb->mock_last_size = req.size;

    bool ok = (tb->mock_err_on_req == 0) || (tb->mock_req_count != tb->mock_err_on_req);

    bti_rsp_if_t rsp = {
        .trans_id = req.trans_id,
        .data = (req.cmd == BTI_REQ_CMD_READ) ? (req.addr + tb->mock_rd_data_off) : 0,
        .ok = ok
    };
    itf_write(&tb->bti_rsp_itf, &rsp);
}

static void tb_clock(axi2bti_tb_t *tb)
{
    axi2bti_clock(&tb->dut);
    tb_mock_bti_slave(tb);

    itf_dbg_clock(&tb->axi4_ar_itf);
    itf_dbg_clock(&tb->axi4_r_itf);
    itf_dbg_clock(&tb->axi4_aw_itf);
    itf_dbg_clock(&tb->axi4_w_itf);
    itf_dbg_clock(&tb->axi4_b_itf);
    itf_dbg_clock(&tb->bti_req_itf);
    itf_dbg_clock(&tb->bti_rsp_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free(axi2bti_tb_t *tb)
{
    axi2bti_free(&tb->dut);

    itf_free(&tb->axi4_ar_itf);
    itf_free(&tb->axi4_r_itf);
    itf_free(&tb->axi4_aw_itf);
    itf_free(&tb->axi4_w_itf);
    itf_free(&tb->axi4_b_itf);
    itf_free(&tb->bti_req_itf);
    itf_free(&tb->bti_rsp_itf);
}

static void tb_axi4_write_ar(axi2bti_tb_t *tb, u8 id, u32 addr, u8 len, u8 size, u8 burst)
{
    axi4_ar_if_t ar = {
        .id = id, .addr = addr, .len = len, .size = size, .burst = burst,
        .lock = false, .cache = 0, .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->axi4_ar_itf, &ar);
}

static void tb_axi4_write_aw(axi2bti_tb_t *tb, u8 id, u32 addr, u8 len, u8 size, u8 burst)
{
    axi4_aw_if_t aw = {
        .id = id, .addr = addr, .len = len, .size = size, .burst = burst,
        .lock = false, .cache = 0, .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->axi4_aw_itf, &aw);
}

static void tb_axi4_write_w(axi2bti_tb_t *tb, u32 data, u8 strb, bool last)
{
    axi4_w_if_t w = { .data = data, .strb = strb, .last = last };
    itf_write(&tb->axi4_w_itf, &w);
}

static bool tb_cond_r_ready(axi2bti_tb_t *tb)
{
    return !itf_fifo_empty(&tb->axi4_r_itf);
}

static bool tb_cond_b_ready(axi2bti_tb_t *tb)
{
    return !itf_fifo_empty(&tb->axi4_b_itf);
}

static bool tb_cond_ar_empty(axi2bti_tb_t *tb)
{
    return itf_fifo_empty(&tb->axi4_ar_itf);
}

static bool tb_cond_aw_empty(axi2bti_tb_t *tb)
{
    return itf_fifo_empty(&tb->axi4_aw_itf);
}

static bool tb_cond_bti_req_ready(axi2bti_tb_t *tb)
{
    return !itf_fifo_empty(&tb->bti_req_itf);
}

static u32 tb_fifo_count(itf_t *itf)
{
    return itf->ctx.fifo.pkt_num;
}

static bool tb_cond_two_bti_reqs(axi2bti_tb_t *tb)
{
    return tb_fifo_count(&tb->bti_req_itf) >= 2;
}

static bool tb_cond_rd_table_full_reqs(axi2bti_tb_t *tb)
{
    return tb_fifo_count(&tb->bti_req_itf) >= AXI2BTI_TB_OST_DEPTH;
}

static bool tb_check_and_pop_r(axi2bti_tb_t *tb, u8 id, u32 data, u8 resp, bool last)
{
    if (itf_fifo_empty(&tb->axi4_r_itf)) {
        return false;
    }
    axi4_r_if_t r;
    itf_read(&tb->axi4_r_itf, &r);
    return r.id == id && r.data == data && r.resp == resp && r.last == last;
}

static bool tb_check_and_pop_b(axi2bti_tb_t *tb, u8 id, u8 resp)
{
    if (itf_fifo_empty(&tb->axi4_b_itf)) {
        return false;
    }
    axi4_b_if_t b;
    itf_read(&tb->axi4_b_itf, &b);
    return b.id == id && b.resp == resp;
}

static bool tb_check_and_pop_bti_req(axi2bti_tb_t *tb, bti_req_cmd_t cmd,
    u32 addr, u16 *trans_id)
{
    if (itf_fifo_empty(&tb->bti_req_itf)) {
        return false;
    }

    bti_req_if_t req;
    itf_read(&tb->bti_req_itf, &req);
    if (trans_id != NULL) {
        *trans_id = req.trans_id;
    }
    return req.cmd == cmd && req.addr == addr;
}

static void tb_write_bti_rsp(axi2bti_tb_t *tb, u16 trans_id, u32 data, bool ok)
{
    bti_rsp_if_t rsp = {
        .trans_id = trans_id,
        .data = data,
        .ok = ok
    };
    itf_write(&tb->bti_rsp_itf, &rsp);
}

TEST_CASE(axi2bti_tb_t, single_read)
{
    TEST_BEGIN("Single-Beat Read");
    tb_reset(tb);

    tb->mock_rd_data_off = 0x1000;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_ar(tb, 0x05, 0x80001000, 0, AXI4_AR_SIZE_B1, 1);

    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x05, 0x80001000 + 0x1000, 0, true),
            "R: id=0x05 data=addr+0x1000 resp=OKAY last=1");
    REQUIRE(tb->mock_req_count == 1, "exactly 1 BTI read");
    REQUIRE(!tb->mock_last_write && tb->mock_last_addr == 0x80001000,
            "BTI read at correct addr");
    REQUIRE(tb->mock_last_size == BTI_REQ_SIZE_B1, "BTI read preserves AXI byte size");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, single_write)
{
    TEST_BEGIN("Single-Beat Write");
    tb_reset(tb);

    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_aw(tb, 0x03, 0x90000000, 0, AXI4_AW_SIZE_B2, 1);
    tb_axi4_write_w(tb, 0x12345678, 0x03, true);

    RUN_POLL_UNTIL(tb_cond_b_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_b(tb, 0x03, 0), "B: id=0x03 resp=OKAY");
    REQUIRE(tb->mock_req_count == 1, "exactly 1 BTI write");
    REQUIRE(tb->mock_last_write && tb->mock_last_data == 0x12345678,
            "BTI write with correct data");
    REQUIRE(tb->mock_last_size == BTI_REQ_SIZE_B2, "BTI write preserves AXI halfword size");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, read_burst)
{
    TEST_BEGIN("Read Burst (len=3, 4 beats, INCR)");
    tb_reset(tb);

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_ar(tb, 0x01, 0x10000000, 3, 2, 1);

    u32 addrs[] = { 0x10000000, 0x10000004, 0x10000008, 0x1000000c };
    for (u32 i = 0; i < 4; i++) {
        RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
        REQUIRE(tb_check_and_pop_r(tb, 0x01, addrs[i], 0, (i == 3)),
                i == 3 ? "R beat[3]: last=1" : "R beat: data ok");
    }
    REQUIRE(tb->mock_req_count == 4, "exactly 4 BTI reads");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, read_burst_fixed)
{
    TEST_BEGIN("Read Burst (len=1, 2 beats, FIXED)");
    tb_reset(tb);

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_ar(tb, 0x02, 0x20000000, 1, 2, 0);

    for (u32 i = 0; i < 2; i++) {
        RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
        tb_check_and_pop_r(tb, 0x02, 0x20000000, 0, (i == 1));
    }
    REQUIRE(tb->mock_req_count == 2, "2 BTI reads");
    REQUIRE(tb->mock_last_addr == 0x20000000, "FIXED: both beats same addr");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, write_burst)
{
    TEST_BEGIN("Write Burst (len=2, 3 beats, INCR)");
    tb_reset(tb);

    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_aw(tb, 0x07, 0x30000000, 2, 2, 1);

    u32 wdata[] = { 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc };
    for (u32 i = 0; i < 3; i++) {
        tb_axi4_write_w(tb, wdata[i], 0x0f, (i == 2));
    }

    RUN_POLL_UNTIL(tb_cond_b_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_b(tb, 0x07, 0), "B: id=0x07 resp=OKAY");
    REQUIRE(tb->mock_req_count == 3, "exactly 3 BTI writes");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, read_burst_bti_error)
{
    TEST_BEGIN("Read Burst BTI Error on Beat 2");
    tb_reset(tb);

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 2;
    tb->mock_req_count = 0;

    tb_axi4_write_ar(tb, 0x0a, 0x40000000, 2, 2, 1);

    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x0a, 0x40000000, 0, false),
            "beat 0: R ok, last=0");
    REQUIRE(tb->mock_req_count == 2, "2 BTI reads issued (beats 0 and 1)");
    REQUIRE(tb->mock_last_addr == 0x40000004, "2nd BTI read at correct addr");

    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x0a, 0, 2, true),
            "beat 1: R SLVERR, last=1 (burst aborted)");
    REQUIRE(tb->mock_req_count == 2, "only 2 BTI reads (beat 0 ok, beat 1 error)");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, write_bti_error)
{
    TEST_BEGIN("Write BTI Error");
    tb_reset(tb);

    tb->mock_err_on_req = 1;
    tb->mock_req_count = 0;

    tb_axi4_write_aw(tb, 0x0b, 0x50000000, 0, 2, 1);
    tb_axi4_write_w(tb, 0xdeadbeef, 0x0f, true);

    RUN_POLL_UNTIL(tb_cond_b_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_b(tb, 0x0b, 2), "B: id=0x0b resp=SLVERR");
    REQUIRE(tb->mock_req_count == 1, "1 BTI write");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, back_to_back)
{
    TEST_BEGIN("Back-to-Back Reads");
    tb_reset(tb);

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;
    tb->mock_auto_rsp = false;

    tb_axi4_write_ar(tb, 0x10, 0x60000000, 0, 2, 1);
    tb_axi4_write_ar(tb, 0x11, 0x70000000, 0, 2, 1);
    RUN_POLL_UNTIL(tb_cond_two_bti_reqs, UT_TIMEOUT);
    REQUIRE(tb_cond_ar_empty(tb), "both ARs accepted before any BTI response");
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == 2,
            "two BTI reads issued before responses");

    u16 tid0;
    u16 tid1;
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x60000000, &tid0),
            "1st BTI read addr");
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x70000000, &tid1),
            "2nd BTI read addr");

    tb_write_bti_rsp(tb, tid0, 0x60000000, true);
    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x10, 0x60000000, 0, true),
            "1st R received");

    tb_write_bti_rsp(tb, tid1, 0x70000000, true);
    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x11, 0x70000000, 0, true), "2nd R received");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, same_id_read_order)
{
    TEST_BEGIN("Same-ID Read Responses Stay Ordered");
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    tb_axi4_write_ar(tb, 0x22, 0x61000000, 0, 2, 1);
    tb_axi4_write_ar(tb, 0x22, 0x61000004, 0, 2, 1);
    RUN_POLL_UNTIL(tb_cond_two_bti_reqs, UT_TIMEOUT);

    u16 tid0;
    u16 tid1;
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x61000000, &tid0),
            "older same-ID BTI read issued");
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x61000004, &tid1),
            "younger same-ID BTI read issued");

    tb_write_bti_rsp(tb, tid1, 0x22220004, true);
    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->axi4_r_itf) == 0,
            "younger same-ID response waits for older response");

    tb_write_bti_rsp(tb, tid0, 0x22220000, true);
    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x22, 0x22220000, 0, true),
            "older same-ID R returned first");

    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x22, 0x22220004, 0, true),
            "younger same-ID R returned second");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, different_id_read_reorder)
{
    TEST_BEGIN("Different-ID Read Responses May Reorder");
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    tb_axi4_write_ar(tb, 0x31, 0x62000000, 0, 2, 1);
    tb_axi4_write_ar(tb, 0x32, 0x62000004, 0, 2, 1);
    RUN_POLL_UNTIL(tb_cond_two_bti_reqs, UT_TIMEOUT);

    u16 tid0;
    u16 tid1;
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x62000000, &tid0),
            "older different-ID BTI read issued");
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x62000004, &tid1),
            "younger different-ID BTI read issued");

    tb_write_bti_rsp(tb, tid1, 0x32323232, true);
    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x32, 0x32323232, 0, true),
            "younger different-ID R can return first");

    tb_write_bti_rsp(tb, tid0, 0x31313131, true);
    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x31, 0x31313131, 0, true),
            "older different-ID R returns when ready");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, read_table_full_delayed_free)
{
    TEST_BEGIN("Read OST Full Uses Next-Cycle Free");
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    for (u32 i = 0; i < AXI2BTI_TB_OST_DEPTH + 1u; i++) {
        tb_axi4_write_ar(tb, 0x40, 0x63000000 + i * 4u, 0, 2, 1);
    }

    RUN_POLL_UNTIL(tb_cond_rd_table_full_reqs, UT_TIMEOUT);
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == AXI2BTI_TB_OST_DEPTH,
            "read table depth worth of BTI reads issued");
    REQUIRE(tb_fifo_count(&tb->axi4_ar_itf) == 1,
            "one AR remains back-pressured while read table is full");

    u16 tid0;
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_READ, 0x63000000, &tid0),
            "oldest BTI read popped for response");
    tb_write_bti_rsp(tb, tid0, 0x63000000, true);
    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->axi4_ar_itf) == 0,
            "held AR is captured into ingress FIFO");
    REQUIRE(fifo_count(&tb->dut.ar_fifo) == 1,
            "captured AR waits in ingress until next ost clock");
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == AXI2BTI_TB_OST_DEPTH - 1u,
            "same-cycle free does not issue captured AR to BTI");

    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->axi4_ar_itf) == 0,
            "host AR interface remains drained after free");
    REQUIRE(fifo_count(&tb->dut.ar_fifo) == 0,
            "held AR issued from ingress on the cycle after free");
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == AXI2BTI_TB_OST_DEPTH,
            "BTI receives held AR after ost clock releases slot");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, write_aw_queue)
{
    TEST_BEGIN("Write AW Queues Behind Pending BTI Write");
    tb_reset(tb);

    tb->mock_auto_rsp = false;

    tb_axi4_write_aw(tb, 0x51, 0x64000000, 0, 2, 1);
    tb_axi4_write_w(tb, 0x11111111, 0x0f, true);
    tb_axi4_write_aw(tb, 0x52, 0x64000004, 0, 2, 1);
    tb_axi4_write_w(tb, 0x22222222, 0x0f, true);

    RUN_POLL_UNTIL(tb_cond_bti_req_ready, UT_TIMEOUT);
    RUN_POLL_UNTIL(tb_cond_aw_empty, UT_TIMEOUT);
    REQUIRE(tb_fifo_count(&tb->axi4_aw_itf) == 0,
            "both AWs accepted while first BTI write is pending");
    REQUIRE(tb_fifo_count(&tb->bti_req_itf) == 1,
            "only head write beat issued before first BTI response");

    u16 tid0;
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_WRITE, 0x64000000, &tid0),
            "first write beat issued");
    tb_write_bti_rsp(tb, tid0, 0, true);
    RUN_POLL_UNTIL(tb_cond_b_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_b(tb, 0x51, 0), "first B returned");

    RUN_POLL_UNTIL(tb_cond_bti_req_ready, UT_TIMEOUT);
    u16 tid1;
    REQUIRE(tb_check_and_pop_bti_req(tb, BTI_REQ_CMD_WRITE, 0x64000004, &tid1),
            "second write beat issued after first completes");
    tb_write_bti_rsp(tb, tid1, 0, true);
    RUN_POLL_UNTIL(tb_cond_b_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_b(tb, 0x52, 0), "second B returned");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, wrap_burst_read)
{
    TEST_BEGIN("WRAP Burst Read -> SLVERR");
    tb_reset(tb);

    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_ar(tb, 0x20, 0x80000000, 3, 2, 2);
    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x20, 0, 2, true),
            "R: id=0x20 resp=SLVERR last=1");
    REQUIRE(tb->mock_req_count == 0, "no BTI transactions");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, wrap_burst_write)
{
    TEST_BEGIN("WRAP Burst Write -> SLVERR");
    tb_reset(tb);

    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_aw(tb, 0x21, 0x90000000, 1, 2, 2);
    RUN_POLL_UNTIL(tb_cond_b_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_b(tb, 0x21, 2), "B: id=0x21 resp=SLVERR");
    REQUIRE(tb->mock_req_count == 0, "no BTI transactions");

    tb_drain_fifos(tb);
    TEST_END();
}

int main()
{
    axi2bti_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(single_read);
    TEST_RUN(single_write);
    TEST_RUN(read_burst);
    TEST_RUN(read_burst_fixed);
    TEST_RUN(write_burst);
    TEST_RUN(read_burst_bti_error);
    TEST_RUN(write_bti_error);
    TEST_RUN(back_to_back);
    TEST_RUN(same_id_read_order);
    TEST_RUN(different_id_read_reorder);
    TEST_RUN(read_table_full_delayed_free);
    TEST_RUN(write_aw_queue);
    TEST_RUN(wrap_burst_read);
    TEST_RUN(wrap_burst_write);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);

    return ret;
}
