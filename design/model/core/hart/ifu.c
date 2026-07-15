#include "ifu.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "dbg/pcm.h"

void ifu_construct(ifu_t *ifu, const char *parent, const char *name,
    const ifu_conf_t *conf)
{
    mod_construct(&ifu->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(conf);
    DBG_CHECK(conf->ctrlq_depth > 0);
    DBG_CHECK(conf->fch_ost_depth > 0);
    DBG_CHECK(ifu->bpu_pred_req_mst != NULL);
    DBG_CHECK(ifu->bpu_pred_rsp_slv != NULL);
    DBG_CHECK(ifu->bpu_update_mst != NULL);

    ifu->reset_pc = conf->reset_pc;
    ifu->ctrlq_depth = conf->ctrlq_depth;
    ifu->fch_ost_depth = conf->fch_ost_depth;
    ifu->boot_rom_info.base = conf->boot_rom_base;
    ifu->boot_rom_info.size = conf->boot_rom_size;
    ifu->exu_state_i = ifu->exu_state_in == NULL ? NULL :
        itf_signal_get_src_and_chk(ifu->exu_state_in);

    fifo_construct(&ifu->ctrlq, sizeof(ifu_ctrlq_entry_t), ifu->ctrlq_depth);
    fifo_construct(&ifu->fch_rspq, sizeof(ifu_fch_rspq_entry_t),
        ifu->fch_ost_depth);
    ostq_construct(&ifu->fch_ost, sizeof(ifu_fch_ost_ctx_t),
        ifu->fch_ost_depth);

    ifu->perf.fch_rsp_inst = dbg_pcm_reg_perf_cnt(ifu->mod.hier_name,
        "fch_rsp_inst");
    ifu->perf.fch_ost_full = dbg_pcm_reg_perf_cnt(ifu->mod.hier_name,
        "fch_ost_full");

    dbg_vcd_add_sig("fch_pc", DBG_SIG_TYPE_REG, 32, &ifu->fch.pc);
    dbg_vcd_add_sig("fch_state", DBG_SIG_TYPE_REG, 2, &ifu->fch.state);
    dbg_vcd_add_sig("fch_epoch", DBG_SIG_TYPE_REG, 32, &ifu->fch.epoch);
    dbg_vcd_add_sig("fch_ost_count", DBG_SIG_TYPE_REG, 32,
        &ifu->fch_ost.count);
    dbg_vcd_add_sig("fch_rspq_num", DBG_SIG_TYPE_REG, 32,
        &ifu->fch_rspq.num);
    dbg_vcd_add_sig("issue_vld", DBG_SIG_TYPE_REG, 1, &ifu->issue.vld);
    dbg_vcd_add_sig("issue_pc", DBG_SIG_TYPE_REG, 32, &ifu->issue.pc);
    dbg_vcd_add_sig("redirect_state", DBG_SIG_TYPE_REG, 2, &ifu->redirect.state);
    dbg_vcd_add_sig("redirect_pc", DBG_SIG_TYPE_REG, 32, &ifu->redirect.pc);
    dbg_vcd_add_sig("ctrlq_entry_num", DBG_SIG_TYPE_REG, 32, &ifu->ctrlq.num);
}

void ifu_reset(ifu_t *ifu)
{
    mod_reset(&ifu->mod);
    ifu->fch.state = IFU_FCH_STATE_REQ;
    ifu->fch.pc = ifu->reset_pc;
    ifu->fch.ir = 0;
    ifu->fch.epoch = 0;
    ifu->back_blocked = false;
    ostq_reset(&ifu->fch_ost);
    fifo_reset(&ifu->fch_rspq);

    ifu->issue.vld = false;
    ifu->issue.pc = ifu->reset_pc;
    ifu->issue.ir = 0;
    ifu->issue.pred_taken = false;
    ifu->issue.pred_target_pc = ifu->reset_pc + 4;
    ifu->issue.pred_vld = false;
    ifu->issue.pred_redirected = false;
    ifu->issue.is_ctrl = false;
    ifu->issue.pred_cond_bht_hit = false;
    ifu->issue.pred_jalr_ras_hit = false;
    ifu->issue.pred_jalr_btb_hit = false;
    ifu->issue.pred_jalr_btb_miss = false;

    ifu->redirect.state = IFU_REDIRECT_STATE_IDLE;
    ifu->redirect.pc = ifu->reset_pc;

    fifo_reset(&ifu->ctrlq);

    *ifu->perf.fch_rsp_inst = 0;
    *ifu->perf.fch_ost_full = 0;

    bpu_pred_req_if_t pred_req = {};
    itf_write(ifu->bpu_pred_req_mst, &pred_req);
    bpu_update_if_t update = {};
    itf_write(ifu->bpu_update_mst, &update);
}

void ifu_free(ifu_t *ifu)
{
    mod_free(&ifu->mod);
    fifo_free(&ifu->ctrlq);
    fifo_free(&ifu->fch_rspq);
    ostq_free(&ifu->fch_ost);
}

static void ifu_clear_bpu_pred_req(ifu_t *ifu)
{
    bpu_pred_req_if_t pred_req = {};
    itf_write(ifu->bpu_pred_req_mst, &pred_req);
}

static void ifu_clear_bpu_update(ifu_t *ifu)
{
    bpu_update_if_t update = {};
    itf_write(ifu->bpu_update_mst, &update);
}

static void ifu_send_bpu_pred_req(ifu_t *ifu, u32 pc, u32 ir)
{
    bpu_pred_req_if_t pred_req = {
        .vld = true,
        .pc = pc,
        .ir = ir
    };
    itf_write(ifu->bpu_pred_req_mst, &pred_req);
}

static void ifu_update_fch_state(ifu_t *ifu)
{
    if (ifu->fch.state == IFU_FCH_STATE_FAULT) {
        return;
    }

    if (!fifo_empty(&ifu->fch_rspq)) {
        ifu->fch.state = IFU_FCH_STATE_RSP;
        return;
    }

    if (!ostq_empty(&ifu->fch_ost)) {
        ifu->fch.state = IFU_FCH_STATE_PEND;
        return;
    }

    ifu->fch.state = IFU_FCH_STATE_REQ;
}

static void ifu_flush_fetch_path(ifu_t *ifu, u32 pc)
{
    ifu->fch.epoch++;
    ifu->fch.pc = pc;
    ifu->fch.ir = 0;
    ifu->fch.state = IFU_FCH_STATE_REQ;
    fifo_clear(&ifu->fch_rspq);
}

static void ifu_fault_fetch_path(ifu_t *ifu, u32 pc)
{
    ifu->fch.epoch++;
    ifu->fch.pc = pc;
    ifu->fch.ir = 0;
    ifu->fch.state = IFU_FCH_STATE_FAULT;
    fifo_clear(&ifu->fch_rspq);
}

static bool ifu_fch_ost_full(ifu_t *ifu)
{
    return ostq_count(&ifu->fch_ost) + fifo_count(&ifu->fch_rspq) >=
        ifu->fch_ost_depth;
}

static void ifu_request_redirect(ifu_t *ifu, u32 pc, ifu_redirect_state_t state)
{
    DBG_CHECK(state == IFU_REDIRECT_STATE_BRANCH || state == IFU_REDIRECT_STATE_TRAP);

    ifu->redirect.pc = pc;
    ifu->redirect.state = state;
}

static void ifu_ctrlq_push(ifu_t *ifu, u32 pc, u32 ir, bool pred_taken,
    u32 pred_pc, bool pred_cond_bht_hit, bool pred_jalr_ras_hit,
    bool pred_jalr_btb_hit, bool pred_jalr_btb_miss)
{
    ifu_ctrlq_entry_t entry = {
        .vld = true,
        .pc = pc,
        .ir = ir,
        .pred_taken = pred_taken,
        .pred_pc = pred_pc,
        .pred_cond_bht_hit = pred_cond_bht_hit,
        .pred_jalr_ras_hit = pred_jalr_ras_hit,
        .pred_jalr_btb_hit = pred_jalr_btb_hit,
        .pred_jalr_btb_miss = pred_jalr_btb_miss
    };
    fifo_push(&ifu->ctrlq, &entry);
}

static bool ifu_ctrlq_pop_valid(ifu_t *ifu, u32 pc, ifu_ctrlq_entry_t *out)
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
    if (out != NULL) {
        *out = entry;
    }

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

    ifu->issue.vld = false;
    ifu->issue.is_ctrl = false;
    ifu->issue.pred_vld = false;
    ifu->issue.pred_redirected = false;
    ifu->fch.ir = 0;

    ifu_flush_fetch_path(ifu, ifu->redirect.pc);
    ifu->redirect.state = IFU_REDIRECT_STATE_IDLE;
    return true;
}

static bool ifu_flush_event_pending(ifu_t *ifu)
{
    return (ifu->tlb_flush_slv != NULL && !itf_fifo_empty(ifu->tlb_flush_slv)) ||
        (ifu->l1i_flush_slv != NULL && !itf_fifo_empty(ifu->l1i_flush_slv));
}

static bool ifu_proc_frontend_flush(ifu_t *ifu)
{
    if (!ifu_flush_event_pending(ifu)) {
        return false;
    }

    if (ifu->exu_state_i == NULL) {
        return false;
    }

    ifu->issue.vld = false;
    ifu->issue.is_ctrl = false;
    ifu->issue.pred_vld = false;
    ifu->issue.pred_redirected = false;
    ifu_ctrlq_flush(ifu);
    ifu_flush_fetch_path(ifu, ifu->exu_state_i->irq_epc);
    return true;
}

static void ifu_send_fch(ifu_t *ifu)
{
    if (ifu->fch.state == IFU_FCH_STATE_FAULT) {
        return;
    }

    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (ifu->issue.vld && !ifu->issue.pred_vld) {
        return;
    }

    if (ifu->issue.vld && ifu->issue.pred_vld &&
        ifu->issue.is_ctrl && ifu->issue.pred_taken &&
        !ifu->issue.pred_redirected) {
        return;
    }

    if (ifu_fch_ost_full(ifu)) {
        (*ifu->perf.fch_ost_full)++;
        return;
    }

    if (ostq_full(&ifu->fch_ost)) {
        (*ifu->perf.fch_ost_full)++;
        return;
    }

    if (itf_fifo_full(ifu->fch_req_mst)) {
        return;
    }

    fch_req_if_t fch_req = {};
    fch_req.pc = ifu->fch.pc;

    ifu_fch_ost_ctx_t ctx = {
        .pc = ifu->fch.pc,
        .epoch = ifu->fch.epoch
    };
    bool alloc_ok = ostq_alloc(&ifu->fch_ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);

    itf_write(ifu->fch_req_mst, &fch_req);
    ifu->fch.pc += 4;
    ifu->fch.state = IFU_FCH_STATE_PEND;
}

static void ifu_proc_fch_rsp(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->fch_rsp_slv)) {
        return;
    }

    ifu_fch_ost_ctx_t ctx;
    bool found = ostq_peek_head(&ifu->fch_ost, &ctx, NULL);
    if (!found) {
        return;
    }

    fch_rsp_if_t fch_rsp;
    itf_fifo_get_front(ifu->fch_rsp_slv, &fch_rsp);

    if (ctx.epoch != ifu->fch.epoch) {
        itf_fifo_pop_front(ifu->fch_rsp_slv);
        (*ifu->perf.fch_rsp_inst)++;
        ostq_free_head(&ifu->fch_ost);
        return;
    }

    if (fifo_full(&ifu->fch_rspq)) {
        return;
    }

    itf_fifo_pop_front(ifu->fch_rsp_slv);
    (*ifu->perf.fch_rsp_inst)++;

    ifu_fch_rspq_entry_t entry = {
        .pc = ctx.pc,
        .ir = fch_rsp.ir,
        .ok = fch_rsp.ok,
        .expt = fch_rsp.expt,
        .cause = fch_rsp.cause,
        .priv = fch_rsp.priv,
        .tval = fch_rsp.tval
    };
    fifo_push(&ifu->fch_rspq, &entry);
    ostq_free_head(&ifu->fch_ost);

    ifu->fch.ir = fch_rsp.ir;
    ifu->fch.state = IFU_FCH_STATE_RSP;
}

static void ifu_apply_bpu_pred_rsp(ifu_t *ifu)
{
    if (!ifu->issue.vld || ifu->issue.pred_vld) {
        return;
    }

    bpu_pred_rsp_if_t rsp;
    itf_read(ifu->bpu_pred_rsp_slv, &rsp);
    DBG_CHECK(rsp.vld);

    ifu->issue.is_ctrl = rsp.is_ctrl;
    ifu->issue.pred_taken = rsp.pred_taken;
    ifu->issue.pred_target_pc = rsp.pred_pc;
    ifu->issue.pred_cond_bht_hit = rsp.cond_bht_hit;
    ifu->issue.pred_jalr_ras_hit = rsp.jalr_ras_hit;
    ifu->issue.pred_jalr_btb_hit = rsp.jalr_btb_hit;
    ifu->issue.pred_jalr_btb_miss = rsp.jalr_btb_miss;
    ifu->issue.pred_vld = true;

    if (ifu->issue.is_ctrl && ifu->issue.pred_taken) {
        ifu_flush_fetch_path(ifu, ifu->issue.pred_target_pc);
        ifu->issue.pred_redirected = true;
    }
}

static void ifu_prepare_issue(ifu_t *ifu)
{
    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (fifo_empty(&ifu->fch_rspq)) {
        return;
    }

    if (ifu->issue.vld) {
        return;
    }

    ifu_fch_rspq_entry_t fch;
    fifo_get_front(&ifu->fch_rspq, &fch);

    if (fch.expt) {
        if (!fifo_empty(&ifu->ctrlq)) {
            return;
        }

        if (itf_fifo_full(ifu->fch_expt_mst)) {
            return;
        }

        fifo_pop(&ifu->fch_rspq, &fch);

        hart_expt_if_t expt = {};
        expt.type = HART_EXPT_TYPE_EXCEPTION;
        expt.cause = (hart_expt_cause_t)fch.cause;
        expt.priv = fch.priv;
        expt.pc = fch.pc;
        expt.tval = fch.tval;
        itf_write(ifu->fch_expt_mst, &expt);
        ifu_fault_fetch_path(ifu, fch.pc);
        return;
    }

    if (!fch.ok) {
        fifo_pop(&ifu->fch_rspq, &fch);
        ifu_flush_fetch_path(ifu, fch.pc);
        return;
    }

    fifo_pop(&ifu->fch_rspq, &fch);

    ifu->issue.vld = true;
    ifu->issue.pc = fch.pc;
    ifu->issue.ir = fch.ir;
    ifu->issue.pred_taken = false;
    ifu->issue.pred_target_pc = fch.pc + 4u;
    ifu->issue.pred_vld = false;
    ifu->issue.pred_redirected = false;
    ifu->issue.is_ctrl = false;
    ifu->issue.pred_cond_bht_hit = false;
    ifu->issue.pred_jalr_ras_hit = false;
    ifu->issue.pred_jalr_btb_hit = false;
    ifu->issue.pred_jalr_btb_miss = false;
    ifu_send_bpu_pred_req(ifu, ifu->issue.pc, ifu->issue.ir);

    ifu_update_fch_state(ifu);
}

static void ifu_send_ex_req(ifu_t *ifu)
{
    if (!ifu->issue.vld) {
        return;
    }

    if (!ifu->issue.pred_vld) {
        return;
    }

    if (ifu->redirect.state != IFU_REDIRECT_STATE_IDLE) {
        return;
    }

    if (itf_fifo_full(ifu->ex_req_mst)) {
        return;
    }

    if (ifu->issue.is_ctrl && fifo_full(&ifu->ctrlq)) {
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
        ifu_ctrlq_push(ifu, ifu->issue.pc, ifu->issue.ir,
            ifu->issue.pred_taken, ifu->issue.pred_target_pc,
            ifu->issue.pred_cond_bht_hit, ifu->issue.pred_jalr_ras_hit,
            ifu->issue.pred_jalr_btb_hit, ifu->issue.pred_jalr_btb_miss);
    }

    ifu->issue.vld = false;
    ifu->issue.is_ctrl = false;
    ifu->issue.pred_vld = false;
    ifu->issue.pred_redirected = false;
    ifu->issue.pred_cond_bht_hit = false;
    ifu->issue.pred_jalr_ras_hit = false;
    ifu->issue.pred_jalr_btb_hit = false;
    ifu->issue.pred_jalr_btb_miss = false;
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

    ifu_ctrlq_entry_t ctrl;
    if (!ifu_ctrlq_pop_valid(ifu, ex_rsp.pc, &ctrl)) {
        return;
    }

    bool is_boot = ADDR_IN(ex_rsp.pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);
    bpu_update_if_t update = {
        .vld = true,
        .pc = ctrl.pc,
        .ir = ctrl.ir,
        .taken = ex_rsp.taken,
        .target_pc = ex_rsp.target_pc,
        .pred_taken = ctrl.pred_taken,
        .pred_pc = ctrl.pred_pc,
        .pred_true = ex_rsp.pred_true,
        .is_boot_code = is_boot,
        .pred_cond_bht_hit = ctrl.pred_cond_bht_hit,
        .pred_jalr_ras_hit = ctrl.pred_jalr_ras_hit,
        .pred_jalr_btb_hit = ctrl.pred_jalr_btb_hit,
        .pred_jalr_btb_miss = ctrl.pred_jalr_btb_miss
    };
    itf_write(ifu->bpu_update_mst, &update);

    if (ex_rsp.pred_true) {
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

static void ifu_front_proc(ifu_t *ifu)
{
    mod_clock(&ifu->mod);
    ostq_clock(&ifu->fch_ost);
    ifu->back_blocked = false;
    ifu_clear_bpu_pred_req(ifu);
    ifu_clear_bpu_update(ifu);

    ifu_proc_ex_rsp(ifu);
    ifu_proc_trap_send(ifu);

    if (ifu->redirect.state == IFU_REDIRECT_STATE_BRANCH ||
        ifu->redirect.state == IFU_REDIRECT_STATE_TRAP) {
        ifu_proc_redirect(ifu);
        ifu_update_fch_state(ifu);
        ifu->back_blocked = true;
        return;
    }

    if (ifu_proc_frontend_flush(ifu)) {
        ifu_update_fch_state(ifu);
        ifu->back_blocked = true;
        return;
    }

    ifu_proc_fch_rsp(ifu);
    ifu_prepare_issue(ifu);
}

static void ifu_back_proc(ifu_t *ifu)
{
    if (ifu->back_blocked) {
        return;
    }

    ifu_apply_bpu_pred_rsp(ifu);
    ifu_send_ex_req(ifu);
    ifu_send_fch(ifu);
    ifu_update_fch_state(ifu);
}

void ifu_clock(ifu_t *ifu)
{
    ifu_front_proc(ifu);
    ifu_back_proc(ifu);
}
