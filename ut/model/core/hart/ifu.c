#include <stdio.h>
#include <stdlib.h>
#include "core/hart/ifu.h"
#include "core/hart/bpu.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_IFU_CTRLQ_DEPTH 16u
#define TB_IFU_FCH_OST_DEPTH 8u

static u32 tb_encode_jal(u32 rd, s32 imm)
{
    u32 uimm = (u32)imm;

    return (((uimm >> 20) & 0x1) << 31) |
           (((uimm >> 1) & 0x3ff) << 21) |
           (((uimm >> 11) & 0x1) << 20) |
           (((uimm >> 12) & 0xff) << 12) |
           ((rd & 0x1f) << 7) |
           OPCODE_JAL;
}

static u32 tb_encode_jalr(u32 rd, u32 rs1, s32 imm)
{
    return (((u32)imm & 0xfff) << 20) |
           ((rs1 & 0x1f) << 15) |
           ((rd & 0x1f) << 7) |
           OPCODE_JALR;
}

static u32 tb_cond_bht_idx(u32 pc)
{
    return (pc >> 2) & (BPU_COND_BHT_SIZE - 1u);
}

typedef struct ifu_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t ex_req_itf;
    itf_t ex_rsp_itf;
    itf_t fl_req_itf;
    itf_t bpu_pred_req_sig_itf;
    itf_t bpu_pred_rsp_sig_itf;
    itf_t bpu_update_sig_itf;
    itf_t fch_expt_itf;
    itf_t trap_send_itf;
    itf_t tlb_flush_itf;
    itf_t l1i_flush_itf;
    itf_t exu_state_sig_itf;
    exu_state_if_t *exu_state;

    ifu_t dut;
    bpu_t bpu;

    ut_sbd_t sbd;
} ifu_tb_t;

static void tb_construct(ifu_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    FCH_REQ_IF_CONSTRUCT(tb, fch_req_itf, 1);
    FCH_RSP_IF_CONSTRUCT(tb, fch_rsp_itf, 1);
    EX_REQ_IF_CONSTRUCT(tb, ex_req_itf, 1);
    EX_RSP_IF_CONSTRUCT(tb, ex_rsp_itf, 1);
    FL_REQ_IF_CONSTRUCT(tb, fl_req_itf, 1);
    BPU_PRED_REQ_SIGNAL_IF_CONSTRUCT(tb, bpu_pred_req_sig_itf, false, false);
    BPU_PRED_RSP_SIGNAL_IF_CONSTRUCT(tb, bpu_pred_rsp_sig_itf, false, false);
    BPU_UPDATE_SIGNAL_IF_CONSTRUCT(tb, bpu_update_sig_itf, false, false);
    HART_EXPT_IF_CONSTRUCT(tb, fch_expt_itf, 1);
    TRAP_SEND_IF_CONSTRUCT(tb, trap_send_itf, 1);
    TLB_FLUSH_IF_CONSTRUCT(tb, tlb_flush_itf, 1);
    L1_FLUSH_IF_CONSTRUCT(tb, l1i_flush_itf, 1);
    EXU_STATE_SIGNAL_IF_CONSTRUCT(tb, exu_state_sig_itf, true, false);
    tb->exu_state = itf_signal_get_src_and_chk(&tb->exu_state_sig_itf);

    tb->dut.fch_req_mst = &tb->fch_req_itf;
    tb->dut.fch_rsp_slv = &tb->fch_rsp_itf;
    tb->dut.ex_req_mst = &tb->ex_req_itf;
    tb->dut.ex_rsp_slv = &tb->ex_rsp_itf;
    tb->dut.fl_req_mst = &tb->fl_req_itf;
    tb->dut.bpu_pred_req_mst = &tb->bpu_pred_req_sig_itf;
    tb->dut.bpu_pred_rsp_slv = &tb->bpu_pred_rsp_sig_itf;
    tb->dut.bpu_update_mst = &tb->bpu_update_sig_itf;
    tb->dut.fch_expt_mst = &tb->fch_expt_itf;
    tb->dut.trap_send_slv = &tb->trap_send_itf;
    tb->dut.tlb_flush_slv = &tb->tlb_flush_itf;
    tb->dut.l1i_flush_slv = &tb->l1i_flush_itf;
    tb->dut.exu_state_in = &tb->exu_state_sig_itf;

    tb->dut.mod.cycle = tb->mod.cycle;
    ifu_conf_t conf = {
        .reset_pc = 0x80000000,
        .boot_rom_base = 0x40000000,
        .boot_rom_size = 0x10000,
        .ctrlq_depth = TB_IFU_CTRLQ_DEPTH,
        .fch_ost_depth = TB_IFU_FCH_OST_DEPTH
    };
    ifu_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    tb->bpu.pred_req_slv = &tb->bpu_pred_req_sig_itf;
    tb->bpu.pred_rsp_mst = &tb->bpu_pred_rsp_sig_itf;
    tb->bpu.update_slv = &tb->bpu_update_sig_itf;
    tb->bpu.mod.cycle = tb->mod.cycle;
    bpu_construct(&tb->bpu, tb->mod.hier_name, "u_bpu");

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(ifu_tb_t *tb)
{
    ifu_reset(&tb->dut);
    bpu_reset(&tb->bpu);
    dbg_vcd_reset();
}

static void tb_free(ifu_tb_t *tb)
{
    ifu_free(&tb->dut);
    bpu_free(&tb->bpu);

    itf_free(&tb->fch_req_itf);
    itf_free(&tb->fch_rsp_itf);
    itf_free(&tb->ex_req_itf);
    itf_free(&tb->ex_rsp_itf);
    itf_free(&tb->fl_req_itf);
    itf_free(&tb->bpu_pred_req_sig_itf);
    itf_free(&tb->bpu_pred_rsp_sig_itf);
    itf_free(&tb->bpu_update_sig_itf);
    itf_free(&tb->fch_expt_itf);
    itf_free(&tb->trap_send_itf);
    itf_free(&tb->tlb_flush_itf);
    itf_free(&tb->l1i_flush_itf);
    itf_free(&tb->exu_state_sig_itf);
}

static void tb_clock(ifu_tb_t *tb)
{
    ifu_clock(&tb->dut);
    bpu_clock(&tb->bpu);

    itf_dbg_clock(&tb->fch_req_itf);
    itf_dbg_clock(&tb->fch_rsp_itf);
    itf_dbg_clock(&tb->ex_req_itf);
    itf_dbg_clock(&tb->ex_rsp_itf);
    itf_dbg_clock(&tb->fl_req_itf);
    itf_dbg_clock(&tb->bpu_pred_req_sig_itf);
    itf_dbg_clock(&tb->bpu_pred_rsp_sig_itf);
    itf_dbg_clock(&tb->bpu_update_sig_itf);
    itf_dbg_clock(&tb->fch_expt_itf);
    itf_dbg_clock(&tb->trap_send_itf);
    itf_dbg_clock(&tb->tlb_flush_itf);
    itf_dbg_clock(&tb->l1i_flush_itf);
    itf_dbg_clock(&tb->exu_state_sig_itf);

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
    bpu_reset(&tb->bpu);

    itf_fifo_pop_all(&tb->fch_req_itf);
    itf_fifo_pop_all(&tb->fch_rsp_itf);
    itf_fifo_pop_all(&tb->ex_req_itf);
    itf_fifo_pop_all(&tb->ex_rsp_itf);
    itf_fifo_pop_all(&tb->fl_req_itf);
    itf_reset(&tb->bpu_pred_req_sig_itf);
    itf_reset(&tb->bpu_pred_rsp_sig_itf);
    itf_reset(&tb->bpu_update_sig_itf);
    itf_fifo_pop_all(&tb->fch_expt_itf);
    itf_fifo_pop_all(&tb->trap_send_itf);
    itf_fifo_pop_all(&tb->tlb_flush_itf);
    itf_fifo_pop_all(&tb->l1i_flush_itf);
    tb->exu_state->priv = 3;
    tb->exu_state->pc = 0x80000000;
    tb->exu_state->irq_epc = 0x80000000;
    tb->exu_state->irq_defer = false;
    tb->exu_state->wfi = false;
    tb->exu_state->wfi_resume_pc = 0;
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

TEST_CASE(ifu_tb_t, ras_predicts_return)
{
    TEST_BEGIN("RAS Predicts Return");

    const u32 call_pc = 0x80000000;
    const u32 call_target = 0x80000100;
    const u32 return_pc = call_pc + 4;
    const u32 jal_ra = tb_encode_jal(1, call_target - call_pc);
    const u32 ret = tb_encode_jalr(0, 1, 0);

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == call_pc, "initial fch_req at call pc");
    }

    fch_rsp_if_t call_rsp = { .ir = jal_ra, .ok = true };
    itf_write(&tb->fch_rsp_itf, &call_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == call_pc, "call issued");
        REQUIRE(req.pred_taken, "JAL predicted taken");
        REQUIRE(req.pred_pc == call_target, "JAL predicted direct target");
    }

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == call_target, "fetch redirected to call target");
    }

    ex_rsp_if_t call_ex_rsp = {
        .pc = call_pc,
        .taken = true,
        .pred_true = true,
        .target_pc = call_target
    };
    itf_write(&tb->ex_rsp_itf, &call_ex_rsp);
    tb_clock(tb);
    REQUIRE(tb->bpu.ras.count == 1, "call pushes return pc into RAS");

    fch_rsp_if_t ret_rsp = { .ir = ret, .ok = true };
    itf_write(&tb->fch_rsp_itf, &ret_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == call_target, "return issued");
        REQUIRE(req.pred_taken, "return predicted taken");
        REQUIRE(req.pred_pc == return_pc, "return predicted from RAS");
    }
    REQUIRE(*tb->bpu.perf.ras_pred == 0, "RAS prediction counter waits for update");

    ex_rsp_if_t ret_ex_rsp = {
        .pc = call_target,
        .taken = true,
        .pred_true = true,
        .target_pc = return_pc
    };
    itf_write(&tb->ex_rsp_itf, &ret_ex_rsp);
    tb_clock(tb);
    REQUIRE(*tb->bpu.perf.ras_pred == 1, "RAS prediction counter increments");
    REQUIRE(*tb->bpu.perf.jalr_ras_hit == 1,
        "JALR RAS hit counter increments");
    REQUIRE(tb->bpu.ras.count == 0, "return pops RAS");
    REQUIRE(itf_fifo_empty(&tb->fl_req_itf), "RAS hit avoids redirect flush");

    TEST_END();
}

TEST_CASE(ifu_tb_t, pred_taken_redirects_while_ex_req_blocked)
{
    TEST_BEGIN("Pred Taken Redirects While EX Req Blocked");

    const u32 call_pc = 0x80000000;
    const u32 call_target = 0x80000100;
    const u32 jal_ra = tb_encode_jal(1, call_target - call_pc);

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == call_pc, "initial fch_req at call pc");
    }

    ex_req_if_t busy = {};
    itf_write(&tb->ex_req_itf, &busy);

    fch_rsp_if_t call_rsp = { .ir = jal_ra, .ok = true };
    itf_write(&tb->fch_rsp_itf, &call_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == call_target,
            "predicted-taken target fetch starts before EX accepts");
    }

    TEST_END();
}

TEST_CASE(ifu_tb_t, cond_bht_predicts_taken_target)
{
    TEST_BEGIN("Cond BHT Predicts Taken Target");

    const u32 branch_pc = 0x80000000;
    const u32 branch_ir = 0x00000463;
    rv32g_inst_t branch_inst = { .raw = branch_ir };
    const u32 branch_target = branch_pc + b_imm_decode(&branch_inst).u;
    const u32 bht_idx = tb_cond_bht_idx(branch_pc);

    tb_drain_all(tb);

    tb->bpu.cond_bht[bht_idx].vld = true;
    tb->bpu.cond_bht[bht_idx].counter = 3;

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == branch_pc, "initial fch_req at branch pc");
    }

    fch_rsp_if_t branch_rsp = { .ir = branch_ir, .ok = true };
    itf_write(&tb->fch_rsp_itf, &branch_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pred_taken, "seeded cond BHT predicts taken");
        REQUIRE(req.pred_pc == branch_target, "branch target is decoded");
    }
    REQUIRE(*tb->bpu.perf.cond_bht_hit == 0,
        "cond BHT hit counter waits for update");

    ex_rsp_if_t branch_ex_rsp = {
        .pc = branch_pc,
        .taken = false,
        .pred_true = false,
        .target_pc = branch_pc + 4
    };
    itf_write(&tb->ex_rsp_itf, &branch_ex_rsp);
    tb_clock(tb);

    REQUIRE(*tb->bpu.perf.cond_bht_hit == 1,
        "cond BHT hit counter increments");
    REQUIRE(tb->bpu.cond_bht[bht_idx].counter == 2,
        "not-taken update leaves weak-taken counter");

    TEST_END();
}

TEST_CASE(ifu_tb_t, jalr_btb_predicts_target)
{
    TEST_BEGIN("JALR BTB Predicts Target");

    const u32 jalr_pc = 0x80000000;
    const u32 jalr_target = 0x80000200;
    const u32 jalr = tb_encode_jalr(0, 2, 0);

    tb_drain_all(tb);

    tb->bpu.jalr_btb[0].vld = true;
    tb->bpu.jalr_btb[0].pc = jalr_pc;
    tb->bpu.jalr_btb[0].target_pc = jalr_target;
    tb->bpu.jalr_btb[0].last_used = 1;

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == jalr_pc, "initial fch_req at JALR pc");
    }

    fch_rsp_if_t jalr_rsp = { .ir = jalr, .ok = true };
    itf_write(&tb->fch_rsp_itf, &jalr_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == jalr_pc, "JALR issued");
        REQUIRE(req.pred_taken, "JALR BTB predicts taken");
        REQUIRE(req.pred_pc == jalr_target, "JALR BTB target is used");
    }
    REQUIRE(*tb->bpu.perf.jalr_btb_hit == 0,
        "JALR BTB hit counter waits for update");
    REQUIRE(*tb->bpu.perf.jalr_btb_miss == 0,
        "JALR BTB miss counter stays clear");

    ex_rsp_if_t jalr_ex_rsp = {
        .pc = jalr_pc,
        .taken = true,
        .pred_true = true,
        .target_pc = jalr_target
    };
    itf_write(&tb->ex_rsp_itf, &jalr_ex_rsp);
    tb_clock(tb);
    REQUIRE(*tb->bpu.perf.jalr_btb_hit == 1,
        "JALR BTB hit counter increments");
    REQUIRE(tb->bpu.jalr_btb[0].target_pc == jalr_target,
        "JALR BTB target remains trained");
    REQUIRE(itf_fifo_empty(&tb->fl_req_itf), "JALR BTB hit avoids flush");

    TEST_END();
}

TEST_CASE(ifu_tb_t, fetch_fault_squashed_by_branch)
{
    TEST_BEGIN("Fetch Fault Squashed by Branch Redirect");

    tb_drain_all(tb);

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000000, "initial fch_req at branch pc");
    }

    fch_rsp_if_t rsp_br = { .ir = 0x00000463, .ok = true };
    itf_write(&tb->fch_rsp_itf, &rsp_br);
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
        REQUIRE(req.pc == 0x80000004, "wrong-path fch_req issued");
    }

    fch_rsp_if_t rsp_fault = {
        .ir = 0,
        .ok = false,
        .expt = true,
        .cause = HART_EXPT_CAUSE_INST_PAGE_FAULT,
        .priv = 1,
        .tval = 0x80000004
    };
    itf_write(&tb->fch_rsp_itf, &rsp_fault);
    tb_clock(tb);

    REQUIRE(itf_fifo_empty(&tb->fch_expt_itf),
        "younger fetch fault waits for older branch");

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

    itf_fifo_pop_all(&tb->fch_req_itf);
    RUN_CYCLES(2);
    REQUIRE(itf_fifo_empty(&tb->fch_expt_itf),
        "wrong-path fetch fault is squashed");

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
    REQUIRE(!itf_fifo_empty(&tb->fch_req_itf), "prefetch continues during stall");
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000004, "next fch_req pc = pc+4 during stall");
    }

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

TEST_CASE(ifu_tb_t, fch_ost_multiple_outstanding)
{
    TEST_BEGIN("Fetch OST Multiple Outstanding");

    tb_drain_all(tb);

    for (u32 i = 0; i < 4; i++) {
        RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
        {
            fch_req_if_t req;
            itf_read(&tb->fch_req_itf, &req);
            REQUIRE(req.pc == 0x80000000 + i * 4,
                "fch_req pc follows sequential window");
        }
    }

    REQUIRE(ostq_count(&tb->dut.fch_ost) >= 4,
        "multiple fetch requests remain outstanding");

    TEST_END();
}

TEST_CASE(ifu_tb_t, frontend_flush_squashes_prefetch)
{
    TEST_BEGIN("Frontend Flush Squashes Prefetch");

    tb_drain_all(tb);

    for (u32 i = 0; i < 2; i++) {
        RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
        {
            fch_req_if_t req;
            itf_read(&tb->fch_req_itf, &req);
            REQUIRE(req.pc == 0x80000000 + i * 4,
                "old-path fch_req issued before flush");
        }
    }

    tb->exu_state->irq_epc = 0x80000100;
    tlb_flush_if_t flush = {};
    itf_write(&tb->tlb_flush_itf, &flush);
    tb_clock(tb);
    itf_fifo_pop_all(&tb->tlb_flush_itf);

    fch_rsp_if_t stale_fault = {
        .ir = 0,
        .ok = false,
        .expt = true,
        .cause = HART_EXPT_CAUSE_INST_PAGE_FAULT,
        .priv = 1,
        .tval = 0x80000000
    };
    itf_write(&tb->fch_rsp_itf, &stale_fault);
    tb_clock(tb);
    REQUIRE(itf_fifo_empty(&tb->fch_expt_itf),
        "stale fetch fault is dropped after frontend flush");

    RUN_POLL_UNTIL(tb_cond_fch_req_ready, UT_TIMEOUT);
    {
        fch_req_if_t req;
        itf_read(&tb->fch_req_itf, &req);
        REQUIRE(req.pc == 0x80000100, "fetch restarts at exu irq_epc");
    }

    fch_rsp_if_t stale_ok = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &stale_ok);
    tb_clock(tb);
    REQUIRE(itf_fifo_empty(&tb->ex_req_itf),
        "stale normal fetch response is dropped after frontend flush");

    fch_rsp_if_t new_rsp = { .ir = 0x00000013, .ok = true };
    itf_write(&tb->fch_rsp_itf, &new_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_ex_req_ready, UT_TIMEOUT);
    {
        ex_req_if_t req;
        itf_read(&tb->ex_req_itf, &req);
        REQUIRE(req.pc == 0x80000100, "new-path fetch response is issued");
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
    TEST_RUN(ras_predicts_return);
    TEST_RUN(pred_taken_redirects_while_ex_req_blocked);
    TEST_RUN(cond_bht_predicts_taken_target);
    TEST_RUN(jalr_btb_predicts_target);
    TEST_RUN(fetch_fault_squashed_by_branch);
    TEST_RUN(trap_redirect);
    TEST_RUN(trap_blocks_during_ctrlq);
    TEST_RUN(back_to_back_issue);
    TEST_RUN(fetch_latency);
    TEST_RUN(fch_ost_multiple_outstanding);
    TEST_RUN(frontend_flush_squashes_prefetch);
    TEST_RUN(ex_rsp_latency);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);

    return ut_sbd_ret(&tb.sbd);
}
