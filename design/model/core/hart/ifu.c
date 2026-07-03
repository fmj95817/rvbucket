#include "ifu.h"
#include "spec/core/isa.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "dbg/pcm.h"
#include "dbg/env.h"

void ifu_construct(ifu_t *ifu, const char *name, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size)
{
    DBG_VCD_MODULE_SCOPE(name);

    ifu->reset_pc = reset_pc;
    ifu->boot_rom_info.base = boot_rom_base;
    ifu->boot_rom_info.size = boot_rom_size;

    fifo_construct(&ifu->ctrlq, sizeof(ifu_ctrlq_entry_t), IFU_CTRLQ_SIZE);

    ifu->bpu.enable = !dbg_get_bool_env("BP_OFF");

    ifu->perf.branch = dbg_pcm_reg_perf_cnt("ifu_branch");
    ifu->perf.pred_true = dbg_pcm_reg_perf_cnt("ifu_pred_true");

    dbg_vcd_add_sig("fch_pc", DBG_SIG_TYPE_REG, 32, &ifu->fch.pc);
    dbg_vcd_add_sig("fch_state", DBG_SIG_TYPE_REG, 2, &ifu->fch.state);
    dbg_vcd_add_sig("issue_vld", DBG_SIG_TYPE_REG, 1, &ifu->issue.vld);
    dbg_vcd_add_sig("issue_pc", DBG_SIG_TYPE_REG, 32, &ifu->issue.pc);
    dbg_vcd_add_sig("redirect_state", DBG_SIG_TYPE_REG, 2, &ifu->redirect.state);
    dbg_vcd_add_sig("redirect_pc", DBG_SIG_TYPE_REG, 32, &ifu->redirect.pc);
    dbg_vcd_add_sig("ctrlq_entry_num", DBG_SIG_TYPE_REG, 32, &ifu->ctrlq.num);
}

void ifu_reset(ifu_t *ifu)
{
    ifu->fch.state = IFU_FCH_STATE_REQ;
    ifu->fch.pc = ifu->reset_pc;
    ifu->fch.ir = 0;

    ifu->issue.vld = false;
    ifu->issue.pc = ifu->reset_pc;
    ifu->issue.ir = 0;
    ifu->issue.pred_taken = false;
    ifu->issue.pred_target_pc = ifu->reset_pc + 4;
    ifu->issue.is_ctrl = false;

    ifu->redirect.state = IFU_REDIRECT_STATE_IDLE;
    ifu->redirect.pc = ifu->reset_pc;

    fifo_reset(&ifu->ctrlq);

    ifu->bpu.access_seq = 0;
    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        ifu->bpu.bht[i].vld = false;
        ifu->bpu.bht[i].pc = 0;
        ifu->bpu.bht[i].counter = 0;
        ifu->bpu.bht[i].target_pc = 0;
        ifu->bpu.bht[i].last_used = 0;
    }

    *ifu->perf.branch = 0;
    *ifu->perf.pred_true = 0;
}

void ifu_free(ifu_t *ifu)
{
    fifo_free(&ifu->ctrlq);
}

static void ifu_request_redirect(ifu_t *ifu, u32 pc, ifu_redirect_state_t state)
{
    DBG_CHECK(state == IFU_REDIRECT_STATE_BRANCH || state == IFU_REDIRECT_STATE_TRAP);

    ifu->redirect.pc = pc;
    ifu->redirect.state = state;
}

static void ifu_ctrlq_push(ifu_t *ifu, u32 pc)
{
    ifu_ctrlq_entry_t entry = { .vld = true, .pc = pc };
    fifo_push(&ifu->ctrlq, &entry);
}

static bool ifu_ctrlq_pop_valid(ifu_t *ifu, u32 pc)
{
    if (fifo_empty(&ifu->ctrlq)) {
        return false;
    }

    ifu_ctrlq_entry_t entry;
    fifo_get_front(&ifu->ctrlq, &entry);

    if (!entry.vld) {
        fifo_pop(&ifu->ctrlq, &entry);
        return false;
    }

    if (entry.pc != pc) {
        return false;
    }

    fifo_pop(&ifu->ctrlq, &entry);

    return true;
}

static void ifu_ctrlq_flush(ifu_t *ifu)
{
    fifo_clear(&ifu->ctrlq);
}

static bool ifu_proc_redirect(ifu_t *ifu)
{
    if (ifu->redirect.state == IFU_REDIRECT_STATE_IDLE) {
        return false;
    }

    if (ifu->redirect.state == IFU_REDIRECT_STATE_REDIRECT) {
        return true;
    }

    ifu->issue.vld = false;
    ifu->issue.is_ctrl = false;
    ifu->fch.ir = 0;

    if (ifu->fch.state == IFU_FCH_STATE_PEND) {
        ifu->redirect.state = IFU_REDIRECT_STATE_REDIRECT;
        return true;
    }

    ifu->fch.pc = ifu->redirect.pc;
    ifu->fch.state = IFU_FCH_STATE_REQ;
    ifu->redirect.state = IFU_REDIRECT_STATE_IDLE;
    return true;
}

static void ifu_send_fch(ifu_t *ifu)
{
    if (ifu->fch.state != IFU_FCH_STATE_REQ) {
        return;
    }

    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (ifu->issue.vld) {
        return;
    }

    if (itf_fifo_full(ifu->fch_req_mst)) {
        return;
    }

    fch_req_if_t fch_req = {};
    fch_req.pc = ifu->fch.pc;
    itf_write(ifu->fch_req_mst, &fch_req);
    ifu->fch.state = IFU_FCH_STATE_PEND;
}

static void ifu_proc_fch_rsp(ifu_t *ifu)
{
    if (ifu->fch.state != IFU_FCH_STATE_PEND) {
        return;
    }

    if (itf_fifo_empty(ifu->fch_rsp_slv)) {
        return;
    }

    fch_rsp_if_t fch_rsp;
    itf_fifo_get_front(ifu->fch_rsp_slv, &fch_rsp);

    if (ifu->redirect.state == IFU_REDIRECT_STATE_REDIRECT) {
        itf_fifo_pop_front(ifu->fch_rsp_slv);
        ifu->fch.ir = 0;
        ifu->fch.pc = ifu->redirect.pc;
        ifu->fch.state = IFU_FCH_STATE_REQ;
        ifu->redirect.state = IFU_REDIRECT_STATE_IDLE;
        return;
    }

    if (fch_rsp.expt) {
        if (itf_fifo_full(ifu->fch_expt_mst)) {
            return;
        }

        itf_fifo_pop_front(ifu->fch_rsp_slv);
        hart_expt_if_t expt = {};
        expt.type = HART_EXPT_TYPE_EXCEPTION;
        expt.cause = (hart_expt_cause_t)fch_rsp.cause;
        expt.priv = fch_rsp.priv;
        expt.pc = ifu->fch.pc;
        expt.tval = fch_rsp.tval;
        itf_write(ifu->fch_expt_mst, &expt);
        ifu->fch.state = IFU_FCH_STATE_FAULT;
        return;
    }

    itf_fifo_pop_front(ifu->fch_rsp_slv);

    if (!fch_rsp.ok) {
        ifu->fch.state = IFU_FCH_STATE_REQ;
        return;
    }

    ifu->fch.ir = fch_rsp.ir;
    ifu->fch.state = IFU_FCH_STATE_RSP;
}

static void ifu_bpu_pred(ifu_t *ifu, u32 pc, bool is_branch)
{
    ifu->issue.pred_taken = false;
    ifu->issue.pred_target_pc = pc + 4;

    if (!ifu->bpu.enable) {
        return;
    }

    if (!is_branch) {
        return;
    }

    rv32g_inst_t inst;
    inst.raw = ifu->issue.ir;

    if (inst.base.opcode == OPCODE_JAL) {
        ifu->issue.pred_taken = true;
        ifu->issue.pred_target_pc = pc + j_imm_decode(&inst).u;
        return;
    }

    ifu->bpu.access_seq++;

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (!ifu->bpu.bht[i].vld) {
            continue;
        }
        if (ifu->bpu.bht[i].pc == pc) {
            ifu->bpu.bht[i].last_used = ifu->bpu.access_seq;
            ifu->issue.pred_taken = (ifu->bpu.bht[i].counter >= 2);
            ifu->issue.pred_target_pc = ifu->bpu.bht[i].target_pc;
            return;
        }
    }

    if (inst.base.opcode == OPCODE_BRANCH) {
        i32 offset = b_imm_decode(&inst);
        if (offset.s < 0) {
            ifu->issue.pred_taken = true;
            ifu->issue.pred_target_pc = pc + offset.u;
        }
    }
}

static void ifu_prepare_issue(ifu_t *ifu)
{
    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (ifu->fch.state != IFU_FCH_STATE_RSP) {
        return;
    }

    if (ifu->issue.vld) {
        return;
    }

    ifu->issue.vld = true;
    ifu->issue.pc = ifu->fch.pc;
    ifu->issue.ir = ifu->fch.ir;

    rv32g_inst_t i;
    i.raw = ifu->issue.ir;
    bool is_branch =
        i.base.opcode == OPCODE_JAL ||
        i.base.opcode == OPCODE_JALR ||
        i.base.opcode == OPCODE_BRANCH;

    ifu->issue.is_ctrl = is_branch;
    ifu_bpu_pred(ifu, ifu->issue.pc, is_branch);

    if (is_branch && ifu->issue.pred_taken) {
        ifu->fch.pc = ifu->issue.pred_target_pc;
    } else {
        ifu->fch.pc = ifu->issue.pc + 4;
    }

    ifu->fch.state = IFU_FCH_STATE_REQ;
}

static void ifu_send_ex_req(ifu_t *ifu)
{
    if (!ifu->issue.vld) {
        return;
    }

    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (itf_fifo_full(ifu->ex_req_mst)) {
        return;
    }

    ex_req_if_t ex_req = {};
    ex_req.inst.raw = ifu->issue.ir;
    ex_req.pc = ifu->issue.pc;
    ex_req.pred_pc = ifu->issue.pred_target_pc;
    ex_req.pred_taken = ifu->issue.pred_taken;
    ex_req.is_boot_code = ADDR_IN(ifu->issue.pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);
    itf_write(ifu->ex_req_mst, &ex_req);

    if (ifu->issue.is_ctrl) {
        ifu_ctrlq_push(ifu, ifu->issue.pc);
    }

    ifu->issue.vld = false;
    ifu->issue.is_ctrl = false;
}

static inline void sat_counter_update(u8 *cnt, bool taken)
{
    if (taken) {
        if (*cnt < 3) {
            (*cnt)++;
        }
    } else {
        if (*cnt > 0) {
            (*cnt)--;
        }
    }
}

static void ifu_update_bpu_bht(ifu_t *ifu, const ex_rsp_if_t *ex_rsp)
{
    if (!ifu->bpu.enable) {
        return;
    }

    ifu->bpu.access_seq++;

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (ifu->bpu.bht[i].vld && ifu->bpu.bht[i].pc == ex_rsp->pc) {
            sat_counter_update(&ifu->bpu.bht[i].counter, ex_rsp->taken);
            ifu->bpu.bht[i].target_pc = ex_rsp->target_pc;
            ifu->bpu.bht[i].last_used = ifu->bpu.access_seq;
            return;
        }
    }

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (!ifu->bpu.bht[i].vld) {
            ifu->bpu.bht[i].vld = true;
            ifu->bpu.bht[i].pc = ex_rsp->pc;
            ifu->bpu.bht[i].counter = ex_rsp->taken ? 2 : 1;
            ifu->bpu.bht[i].target_pc = ex_rsp->target_pc;
            ifu->bpu.bht[i].last_used = ifu->bpu.access_seq;
            return;
        }
    }

    u32 lru_idx = 0;
    u32 lru_min = ifu->bpu.bht[0].last_used;
    for (u32 i = 1; i < IFU_BHT_SIZE; i++) {
        if (ifu->bpu.bht[i].last_used < lru_min) {
            lru_idx = i;
            lru_min = ifu->bpu.bht[i].last_used;
        }
    }

    ifu->bpu.bht[lru_idx].vld = true;
    ifu->bpu.bht[lru_idx].pc = ex_rsp->pc;
    ifu->bpu.bht[lru_idx].counter = ex_rsp->taken ? 2 : 1;
    ifu->bpu.bht[lru_idx].target_pc = ex_rsp->target_pc;
    ifu->bpu.bht[lru_idx].last_used = ifu->bpu.access_seq;
}

static void ifu_proc_ex_rsp(ifu_t *ifu)
{
    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (itf_fifo_empty(ifu->ex_rsp_slv)) {
        return;
    }

    ex_rsp_if_t ex_rsp;
    itf_read(ifu->ex_rsp_slv, &ex_rsp);

    if (!ifu_ctrlq_pop_valid(ifu, ex_rsp.pc)) {
        return;
    }

    bool is_boot = ADDR_IN(ex_rsp.pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);

    if (!is_boot) {
        (*ifu->perf.branch)++;
    }

    ifu_update_bpu_bht(ifu, &ex_rsp);

    if (ex_rsp.pred_true) {
        if (!is_boot) {
            (*ifu->perf.pred_true)++;
        }
        return;
    }

    DBG_CHECK(itf_fifo_empty(ifu->fl_req_mst));
    fl_req_if_t fl_req = {};
    itf_write(ifu->fl_req_mst, &fl_req);

    ifu_ctrlq_flush(ifu);
    ifu_request_redirect(ifu, ex_rsp.target_pc, IFU_REDIRECT_STATE_BRANCH);
}

static void ifu_proc_trap_send(ifu_t *ifu)
{
    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (!itf_fifo_empty(ifu->ex_rsp_slv)) {
        return;
    }

    if (!fifo_empty(&ifu->ctrlq)) {
        return;
    }

    if (itf_fifo_empty(ifu->trap_send_slv)) {
        return;
    }

    trap_send_if_t trap_send;
    itf_read(ifu->trap_send_slv, &trap_send);

    DBG_CHECK(itf_fifo_empty(ifu->fl_req_mst));
    fl_req_if_t fl_req = {};
    itf_write(ifu->fl_req_mst, &fl_req);

    ifu_ctrlq_flush(ifu);
    ifu_request_redirect(ifu, trap_send.target_pc, IFU_REDIRECT_STATE_TRAP);
}

void ifu_clock(ifu_t *ifu)
{
    ifu_proc_ex_rsp(ifu);
    ifu_proc_trap_send(ifu);

    if (ifu->redirect.state == IFU_REDIRECT_STATE_BRANCH ||
        ifu->redirect.state == IFU_REDIRECT_STATE_TRAP) {
        ifu_proc_redirect(ifu);
        return;
    }

    if (ifu->redirect.state == IFU_REDIRECT_STATE_REDIRECT) {
        ifu_proc_fch_rsp(ifu);
        return;
    }

    ifu_proc_fch_rsp(ifu);
    ifu_prepare_issue(ifu);
    ifu_send_ex_req(ifu);
    ifu_send_fch(ifu);
}
