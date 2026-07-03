#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus/bridge.h"
#include "utils.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

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

    ut_sbd_t sbd;
} axi2bti_tb_t;

static void tb_construct(axi2bti_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);

    AXI4_AR_IF_CONSTRUCT(tb, axi4_ar_itf, 1);
    AXI4_R_IF_CONSTRUCT(tb, axi4_r_itf, 1);
    AXI4_AW_IF_CONSTRUCT(tb, axi4_aw_itf, 1);
    AXI4_W_IF_CONSTRUCT(tb, axi4_w_itf, 8);
    AXI4_B_IF_CONSTRUCT(tb, axi4_b_itf, 1);
    BTI_REQ_IF_CONSTRUCT(tb, bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(tb, bti_rsp_itf, 1);

    tb->dut.axi4_ar_slv = &tb->axi4_ar_itf;
    tb->dut.axi4_r_mst = &tb->axi4_r_itf;
    tb->dut.axi4_aw_slv = &tb->axi4_aw_itf;
    tb->dut.axi4_w_slv = &tb->axi4_w_itf;
    tb->dut.axi4_b_mst = &tb->axi4_b_itf;
    tb->dut.bti_req_mst = &tb->bti_req_itf;
    tb->dut.bti_rsp_slv = &tb->bti_rsp_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    axi2bti_construct(&tb->dut, tb->mod.hier_name, "u_dut");

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(axi2bti_tb_t *tb)
{
    axi2bti_reset(&tb->dut);
    dbg_vcd_reset();

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;
    tb->mock_last_addr = 0;
    tb->mock_last_data = 0;
    tb->mock_last_write = false;
    tb->mock_last_size = BTI_REQ_SIZE_B1;
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

TEST_CASE(axi2bti_tb_t, single_read)
{
    TEST_BEGIN("Single-Beat Read");

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

    tb->mock_rd_data_off = 0;
    tb->mock_err_on_req = 0;
    tb->mock_req_count = 0;

    tb_axi4_write_ar(tb, 0x10, 0x60000000, 0, 2, 1);
    RUN_POLL_UNTIL(tb_cond_ar_empty, UT_TIMEOUT);

    tb_axi4_write_ar(tb, 0x11, 0x70000000, 0, 2, 1);
    REQUIRE(!itf_fifo_empty(&tb->axi4_ar_itf), "2nd AR queued (DUT busy)");

    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x10, 0x60000000, 0, true), "1st R received");
    REQUIRE(tb->mock_req_count == 1, "1 BTI read for 1st AR");

    RUN_POLL_UNTIL(tb_cond_r_ready, UT_TIMEOUT);
    REQUIRE(tb_check_and_pop_r(tb, 0x11, 0x70000000, 0, true), "2nd R received");
    REQUIRE(tb->mock_req_count == 2, "2 BTI reads total");

    tb_drain_fifos(tb);
    TEST_END();
}

TEST_CASE(axi2bti_tb_t, wrap_burst_read)
{
    TEST_BEGIN("WRAP Burst Read -> SLVERR");

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
    TEST_RUN(wrap_burst_read);
    TEST_RUN(wrap_burst_write);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);

    return ret;
}
