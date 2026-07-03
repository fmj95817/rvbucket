#include <stdio.h>
#include <stdlib.h>
#include "core/hart/ifu.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct ifu_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t ex_req_itf;
    itf_t ex_rsp_itf;
    itf_t fl_req_itf;
    itf_t trap_send_itf;

    ifu_t dut;

    ut_sbd_t sbd;
} ifu_tb_t;

static void tb_construct(ifu_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);

    FCH_REQ_IF_CONSTRUCT(tb, fch_req_itf, 1);
    FCH_RSP_IF_CONSTRUCT(tb, fch_rsp_itf, 1);
    EX_REQ_IF_CONSTRUCT(tb, ex_req_itf, 1);
    EX_RSP_IF_CONSTRUCT(tb, ex_rsp_itf, 1);
    FL_REQ_IF_CONSTRUCT(tb, fl_req_itf, 1);
    TRAP_SEND_IF_CONSTRUCT(tb, trap_send_itf, 1);

    tb->dut.fch_req_mst = &tb->fch_req_itf;
    tb->dut.fch_rsp_slv = &tb->fch_rsp_itf;
    tb->dut.ex_req_mst = &tb->ex_req_itf;
    tb->dut.ex_rsp_slv = &tb->ex_rsp_itf;
    tb->dut.fl_req_mst = &tb->fl_req_itf;
    tb->dut.trap_send_slv = &tb->trap_send_itf;

    tb->dut.mod.cycle = tb->mod.cycle;
    ifu_construct(&tb->dut, tb->mod.hier_name, "u_dut", 0x80000000, 0x40000000, 0x10000);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(ifu_tb_t *tb)
{
    ifu_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_free(ifu_tb_t *tb)
{
    ifu_free(&tb->dut);

    itf_free(&tb->fch_req_itf);
    itf_free(&tb->fch_rsp_itf);
    itf_free(&tb->ex_req_itf);
    itf_free(&tb->ex_rsp_itf);
    itf_free(&tb->fl_req_itf);
    itf_free(&tb->trap_send_itf);
}

static void tb_clock(ifu_tb_t *tb)
{
    ifu_clock(&tb->dut);

    itf_dbg_clock(&tb->fch_req_itf);
    itf_dbg_clock(&tb->fch_rsp_itf);
    itf_dbg_clock(&tb->ex_req_itf);
    itf_dbg_clock(&tb->ex_rsp_itf);
    itf_dbg_clock(&tb->fl_req_itf);
    itf_dbg_clock(&tb->trap_send_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static bool tb_cond_fch_req_ready(ifu_tb_t *tb)
{
    return !itf_fifo_empty(&tb->fch_req_itf);
}

static bool tb_cond_ex_req_ready(ifu_tb_t *tb)
{
    return !itf_fifo_empty(&tb->ex_req_itf);
}

static bool tb_cond_fl_req_ready(ifu_tb_t *tb)
{
    return !itf_fifo_empty(&tb->fl_req_itf);
}

static void tb_drain_all(ifu_tb_t *tb)
{
    ifu_reset(&tb->dut);

    itf_fifo_pop_all(&tb->fch_req_itf);
    itf_fifo_pop_all(&tb->fch_rsp_itf);
    itf_fifo_pop_all(&tb->ex_req_itf);
    itf_fifo_pop_all(&tb->ex_rsp_itf);
    itf_fifo_pop_all(&tb->fl_req_itf);
    itf_fifo_pop_all(&tb->trap_send_itf);
}

TEST_CASE(ifu_tb_t, normal_fetch_issue)
{
    TEST_BEGIN("Normal Fetch and Issue");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "fch_req pc = reset_pc");
    }

    fch_rsp_if_t rsp = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "ex_req pc = 0x80000000");
        REQUIRE(!req.pred_taken, "NOP: pred_taken=false");
    }

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000004, "next fch_req pc = pc+4");
    }

    TEST_END();
}

TEST_CASE(ifu_tb_t, fetch_error_retry)
{
    TEST_BEGIN("Fetch Error Retry");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "initial fch_req at pc");
    }

    fch_rsp_if_t rsp_err = { .ir = 0, .ok = false };
    itf_write(&tb->fch_rsp_itf, &rsp_err);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "fch_req retries same pc after error");
    }

    REQUIRE(itf_fifo_empty(&tb->ex_req_itf), "no ex_req after fetch error");

    TEST_END();
}

TEST_CASE(ifu_tb_t, branch_mispredict)
{
    TEST_BEGIN("Branch Mispredict");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
    }

    fch_rsp_if_t rsp_nop = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_nop);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
    }

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
    }

    fch_rsp_if_t rsp_br = { .ir = 0x00000463, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_br);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == 0x80000004, "ex_req pc = branch pc");
    }

    ex_rsp_if_t ex_rsp = {
        .pc = 0x80000004,
        .taken = true,
        .pred_true = false,
        .target_pc = 0x80001000
    };
    itf_write(&tb->ex_rsp_itf, &ex_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fl_req_ready, UT_TIMEOUT);
    {
        fl_req_if_t fl;
        itf_read(&tb->fl_req_itf, &fl);
    }

    itf_fifo_pop_all(&tb->fch_req_itf);

    fch_rsp_if_t rsp_drain = { .ir = 0, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_drain);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80001000, "redirected fch_req at target_pc");
    }

    TEST_END();
}

TEST_CASE(ifu_tb_t, trap_redirect)
{
    TEST_BEGIN("Trap Redirect");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
    }

    fch_rsp_if_t rsp_nop = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_nop);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
    }

    trap_send_if_t trap = { .target_pc = 0x80010000 };
    itf_write(&tb->trap_send_itf, &trap);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fl_req_ready, UT_TIMEOUT);
    {
        fl_req_if_t fl;
        itf_read(&tb->fl_req_itf, &fl);
    }

    itf_fifo_pop_all(&tb->fch_req_itf);

    fch_rsp_if_t rsp_drain = { .ir = 0, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_drain);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80010000, "trap redirect to target_pc");
    }

    TEST_END();
}

TEST_CASE(ifu_tb_t, trap_blocks_during_ctrlq)
{
    TEST_BEGIN("Trap Blocked During Pending Branch");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
    }

    fch_rsp_if_t rsp_br = { .ir = 0x00000463, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_br);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
    }

    trap_send_if_t trap = { .target_pc = 0x80020000 };
    itf_write(&tb->trap_send_itf, &trap);

    tb_clock(tb);
    tb_clock(tb);
    REQUIRE(itf_fifo_empty(&tb->fl_req_itf), "trap blocked: ctrlq not empty");

    itf_fifo_pop_all(&tb->fch_req_itf);

    ex_rsp_if_t ex_rsp = {
        .pc = 0x80000000,
        .taken = true,
        .pred_true = false,
        .target_pc = 0x80001000
    };
    itf_write(&tb->ex_rsp_itf, &ex_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fl_req_ready, UT_TIMEOUT);
    {
        fl_req_if_t fl;
        itf_read(&tb->fl_req_itf, &fl);
    }

    fch_rsp_if_t rsp_drain = { .ir = 0, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_drain);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fl_req_ready, UT_TIMEOUT);
    {
        fl_req_if_t fl;
        itf_read(&tb->fl_req_itf, &fl);
    }

    TEST_END();
}

TEST_CASE(ifu_tb_t, back_to_back_issue)
{
    TEST_BEGIN("Back-to-Back Issue");

    tb_drain_all(tb);

    for (u32 i = 0; i < 3; i++) {
        RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
        {
            fch_req_if_t req;
            itf_read(&tb->fch_req_itf, &req);
        }

        fch_rsp_if_t rsp = { .ir = 0x00000013, .ok = true };
        itf_write(&tb->fch_rsp_itf, &rsp);
        tb_clock(tb);

        u32 ex_pc;
        RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
        {
            ex_req_if_t req;
            itf_read(&tb->ex_req_itf, &req);
            ex_pc = req.pc;
        }

        ex_rsp_if_t ex_rsp = {
            .pc = ex_pc,
            .taken = false,
            .pred_true = true,
            .target_pc = 0
        };
        itf_write(&tb->ex_rsp_itf, &ex_rsp);
        tb_clock(tb);
    }

    REQUIRE(itf_fifo_empty(&tb->fl_req_itf), "no unexpected flushes");

    TEST_END();
}

TEST_CASE(ifu_tb_t, fetch_latency)
{
    TEST_BEGIN("Fetch Response with Latency");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "fch_req pc = reset_pc");
    }

    RUN_CYCLES(5);
    REQUIRE(itf_fifo_empty(&tb->fch_req_itf), "no extra fch_req during stall");

    fch_rsp_if_t rsp = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "ex_req issued after fetch latency");
    }

    TEST_END();
}

TEST_CASE(ifu_tb_t, ex_rsp_latency)
{
    TEST_BEGIN("EX Response with Latency");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
    }

    fch_rsp_if_t rsp = { .ir = 0x00000463, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "branch issued");
    }

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000004, "prefetch continues while waiting ex_rsp");
    }

    fch_rsp_if_t rsp2 = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp2);
    tb_clock(tb);

    RUN_CYCLES(5);

    ex_rsp_if_t ex_rsp1 = {
        .pc = 0x80000000,
        .taken = false,
        .pred_true = true,
        .target_pc = 0x80000004
    };
    itf_write(&tb->ex_rsp_itf, &ex_rsp1);
    tb_clock(tb);

    REQUIRE(itf_fifo_empty(&tb->fl_req_itf), "no flush (prediction correct)");

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == 0x80000004, "next instr issued after branch resolves");
    }

    TEST_END();
}

int main(void)
{
    ifu_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(normal_fetch_issue);
    TEST_RUN(fetch_error_retry);
    TEST_RUN(branch_mispredict);
    TEST_RUN(trap_redirect);
    TEST_RUN(trap_blocks_during_ctrlq);
    TEST_RUN(back_to_back_issue);
    TEST_RUN(fetch_latency);
    TEST_RUN(ex_rsp_latency);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);

    return ut_sbd_ret(&tb.sbd);
}
