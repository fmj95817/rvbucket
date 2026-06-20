#include "ifu.h"
#include "spec/isa.h"
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

    ifu->bpu.enable = !dbg_get_bool_env("BP_OFF");

    ifu->perf.branch = dbg_pcm_reg_perf_cnt("ifu_branch");
    ifu->perf.pred_true = dbg_pcm_reg_perf_cnt("ifu_pred_true");

    dbg_vcd_add_sig("fch_pc", DBG_SIG_TYPE_REG, 32, &ifu->fch.pc);
    dbg_vcd_add_sig("fch_pend", DBG_SIG_TYPE_REG, 1, &ifu->fch.pend);
    dbg_vcd_add_sig("fch_vld", DBG_SIG_TYPE_REG, 1, &ifu->fch.vld);
    dbg_vcd_add_sig("issue_vld", DBG_SIG_TYPE_REG, 1, &ifu->issue.vld);
    dbg_vcd_add_sig("issue_pc", DBG_SIG_TYPE_REG, 32, &ifu->issue.pc);
    dbg_vcd_add_sig("resume_vld", DBG_SIG_TYPE_REG, 1, &ifu->resume.vld);
    dbg_vcd_add_sig("resume_pc", DBG_SIG_TYPE_REG, 32, &ifu->resume.pc);
}

void ifu_reset(ifu_t *ifu)
{
    ifu->fch.pend = false;
    ifu->fch.vld = false;
    ifu->fch.drop_rsp = false;
    ifu->fch.pc = ifu->reset_pc;
    ifu->fch.ir = 0;

    ifu->issue.vld = false;
    ifu->issue.pc = ifu->reset_pc;
    ifu->issue.ir = 0;
    ifu->issue.pred_taken = false;
    ifu->issue.pred_target_pc = ifu->reset_pc;

    ifu->resume.vld = false;
    ifu->resume.pc = ifu->reset_pc;

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

void ifu_free(ifu_t *ifu) {}

static void ifu_send_fch(ifu_t *ifu)
{
    if (ifu->fch.pend) {
        return;
    }

    if (itf_fifo_full(ifu->fch_req_mst)) {
        return;
    }

    fch_req_if_t fch_req;
    fch_req.pc = ifu->fch.pc;
    itf_write(ifu->fch_req_mst, &fch_req);
    ifu->fch.pend = true;
}

static void ifu_proc_fch_rsp(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->fch_rsp_slv)) {
        return;
    }

    fch_rsp_if_t fch_rsp;
    itf_read(ifu->fch_rsp_slv, &fch_rsp);
    if (ifu->fch.drop_rsp) {
        ifu->fch.drop_rsp = false;
        ifu->fch.pend = false;
        ifu->fch.vld = false;
        return;
    }

    if (!fch_rsp.ok) {
        ifu->fch.pend = false;
        ifu->fch.vld = false;
        return;
    }

    ifu->fch.ir = fch_rsp.ir;
    ifu->fch.vld = true;
}

static s32 decode_b_imm(rv32g_inst_t inst)
{
    u32 imm = (inst.b.imm_12 << 12) | (inst.b.imm_11 << 11) |
              (inst.b.imm_10_5 << 5) | (inst.b.imm_4_1 << 1);
    if (imm & (1u << 12))
        imm |= 0xFFFFE000u;
    return (s32)imm;
}

static u32 decode_jal_imm(rv32g_inst_t inst)
{
    u32 imm = (inst.j.imm_20 << 20) | (inst.j.imm_19_12 << 12) |
              (inst.j.imm_11 << 11) | (inst.j.imm_10_1 << 1);
    if (imm & (1u << 20))
        imm |= 0xFFE00000u;
    return imm;
}

static void ifu_bpu_pred(ifu_t *ifu, u32 pc, bool is_branch)
{
    ifu->issue.pred_taken = false;

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
        ifu->issue.pred_target_pc = pc + decode_jal_imm(inst);
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
        s32 offset = decode_b_imm(inst);
        if (offset < 0) {
            ifu->issue.pred_taken = true;
            ifu->issue.pred_target_pc = pc + (u32)offset;
        }
    }
}

static void ifu_prepare_issue(ifu_t *ifu)
{
    if (!ifu->fch.vld) {
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
    ifu_bpu_pred(ifu, ifu->issue.pc, is_branch);

    u32 pc_nxt;
    if (is_branch && ifu->issue.pred_taken) {
        pc_nxt = ifu->issue.pred_target_pc;
    } else {
        pc_nxt = ifu->fch.pc + 4;
    }

    if (ifu->resume.vld) {
        pc_nxt = ifu->resume.pc;
        ifu->resume.vld = false;
    }

    ifu->fch.pc = pc_nxt;
    ifu->fch.vld = false;
    ifu->fch.pend = false;
}

static void ifu_send_ex_req(ifu_t *ifu)
{
    if (!ifu->issue.vld) {
        return;
    }

    if (itf_fifo_full(ifu->ex_req_mst)) {
        return;
    }

    ex_req_if_t ex_req;
    ex_req.inst.raw = ifu->issue.ir;
    ex_req.pc = ifu->issue.pc;
    ex_req.pred_pc = ifu->issue.pred_target_pc;
    ex_req.pred_taken = ifu->issue.pred_taken;
    ex_req.is_boot_code = ADDR_IN(ifu->issue.pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);
    itf_write(ifu->ex_req_mst, &ex_req);
    ifu->issue.vld = false;
}

static inline void sat_counter_update(u8 *cnt, bool taken)
{
    if (taken) {
        if (*cnt < 3) (*cnt)++;
    } else {
        if (*cnt > 0) (*cnt)--;
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

    ifu->bpu.bht[lru_idx].pc = ex_rsp->pc;
    ifu->bpu.bht[lru_idx].counter = ex_rsp->taken ? 2 : 1;
    ifu->bpu.bht[lru_idx].target_pc = ex_rsp->target_pc;
    ifu->bpu.bht[lru_idx].last_used = ifu->bpu.access_seq;
}

static void ifu_proc_ex_rsp(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->ex_rsp_slv)) {
        return;
    }

    if (ifu->resume.vld) {
        return;
    }

    ex_rsp_if_t ex_rsp;
    itf_read(ifu->ex_rsp_slv, &ex_rsp);

    bool is_boot = ADDR_IN(ex_rsp.pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);

    if (!is_boot) {
        (*ifu->perf.branch)++;
    }

    if (ex_rsp.pred_true) {
        if (!is_boot) {
            (*ifu->perf.pred_true)++;
        }
        return;
    }

    if (itf_fifo_full(ifu->fl_req_mst)) {
        return;
    }

    fl_req_if_t fl_req = {};
    itf_write(ifu->fl_req_mst, &fl_req);

    ifu_update_bpu_bht(ifu, &ex_rsp);

    bool dropped_rsp = false;
    while (!itf_fifo_empty(ifu->fch_rsp_slv)) {
        fch_rsp_if_t dummy;
        itf_read(ifu->fch_rsp_slv, &dummy);
        dropped_rsp = true;
    }

    bool dropped_req = false;
    while (!itf_fifo_empty(ifu->fch_req_mst)) {
        fch_req_if_t dummy;
        itf_read(ifu->fch_req_mst, &dummy);
        dropped_req = true;
    }

    ifu->fch.drop_rsp = ifu->fch.pend && !ifu->fch.vld && !dropped_rsp && !dropped_req;
    ifu->fch.vld = false;
    ifu->fch.pend = ifu->fch.drop_rsp;
    ifu->issue.vld = false;
    ifu->fch.pc = ex_rsp.target_pc;
    ifu->resume.vld = false;
}

static void ifu_proc_trap_send(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->trap_send_slv)) {
        return;
    }

    trap_send_if_t trap_send;
    itf_read(ifu->trap_send_slv, &trap_send);

    bool dropped_rsp = false;
    while (!itf_fifo_empty(ifu->fch_rsp_slv)) {
        fch_rsp_if_t dummy;
        itf_read(ifu->fch_rsp_slv, &dummy);
        dropped_rsp = true;
    }

    bool dropped_req = false;
    while (!itf_fifo_empty(ifu->fch_req_mst)) {
        fch_req_if_t dummy;
        itf_read(ifu->fch_req_mst, &dummy);
        dropped_req = true;
    }

    ifu->fch.drop_rsp = ifu->fch.pend && !ifu->fch.vld && !dropped_rsp && !dropped_req;
    ifu->fch.vld = false;
    ifu->fch.pend = ifu->fch.drop_rsp;
    ifu->issue.vld = false;
    ifu->fch.pc = trap_send.target_pc;
}

void ifu_clock(ifu_t *ifu)
{
    ifu_proc_trap_send(ifu);
    ifu_send_fch(ifu);
    ifu_proc_fch_rsp(ifu);
    ifu_prepare_issue(ifu);
    ifu_send_ex_req(ifu);
    ifu_proc_ex_rsp(ifu);
}
