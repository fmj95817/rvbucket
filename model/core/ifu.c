#include "ifu.h"
#include "mod_if.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "dbg/pcm.h"
#include "dbg/env.h"

void ifu_construct(ifu_t *ifu, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size)
{
    ifu->reset_pc = reset_pc;

    ifu->boot_rom_info.base = boot_rom_base;
    ifu->boot_rom_info.size = boot_rom_size;

    ifu->bpu.enable = dbg_get_bool_env("BR_PRED");

    ifu->perf.branch = dbg_pcm_reg_perf_cnt("ifu_branch");
    ifu->perf.pred_true = dbg_pcm_reg_perf_cnt("ifu_pred_true");

    dbg_vcd_scope_begin("ifu");
    dbg_vcd_add_sig("fch.pc", DBG_SIG_TYPE_REG, 32, &ifu->fch.pc);
    dbg_vcd_add_sig("fch.pend", DBG_SIG_TYPE_REG, 1, &ifu->fch.pend);
    dbg_vcd_add_sig("fch.vld", DBG_SIG_TYPE_REG, 1, &ifu->fch.vld);
    dbg_vcd_add_sig("issue.vld", DBG_SIG_TYPE_REG, 1, &ifu->issue.vld);
    dbg_vcd_add_sig("issue.pc", DBG_SIG_TYPE_REG, 32, &ifu->issue.pc);
    dbg_vcd_add_sig("resume.vld", DBG_SIG_TYPE_REG, 1, &ifu->resume.vld);
    dbg_vcd_add_sig("resume.pc", DBG_SIG_TYPE_REG, 32, &ifu->resume.pc);
    dbg_vcd_scope_end();
}

void ifu_reset(ifu_t *ifu)
{
    ifu->fch.pend = false;
    ifu->fch.vld = false;
    ifu->fch.pc = ifu->reset_pc;
    ifu->fch.ir = 0;

    ifu->issue.vld = false;
    ifu->issue.pc = ifu->reset_pc;
    ifu->issue.ir = 0;
    ifu->issue.pred_taken = false;
    ifu->issue.pred_target_pc = ifu->reset_pc;

    ifu->resume.vld = false;
    ifu->resume.pc = ifu->reset_pc;

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        ifu->bpu.bht[i].vld = false;
        ifu->bpu.bht[i].pc = ifu->reset_pc;
        ifu->bpu.bht[i].taken = false;
        ifu->bpu.bht[i].target_pc = ifu->reset_pc;
        ifu->bpu.bht[i].used_bits = 0;
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
    ifu->fch.ir = fch_rsp.ir;
    ifu->fch.vld = true;
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

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (!ifu->bpu.bht[i].vld) {
            continue;
        }

        ifu->bpu.bht[i].used_bits <<= 1u;
        if (ifu->bpu.bht[i].pc == pc) {
            ifu->bpu.bht[i].used_bits |= 1u;
            ifu->issue.pred_taken = ifu->bpu.bht[i].taken;
            ifu->issue.pred_target_pc = ifu->bpu.bht[i].target_pc;
            return;
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

    rv32i_inst_t i;
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

static inline u32 calc_used_times(u16 used_bits)
{
    u32 n = 0;
    for (u32 i = 0; i < 16; i++) {
        n += ((used_bits & (1u << i)) ? 1 : 0);
    }
    return n;
}

static void ifu_update_bpu_bht(ifu_t *ifu, const ex_rsp_if_t *ex_rsp)
{
    if (!ifu->bpu.enable) {
        return;
    }

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (ifu->bpu.bht[i].vld && ifu->bpu.bht[i].pc == ex_rsp->pc) {
            ifu->bpu.bht[i].taken = ex_rsp->taken;
            ifu->bpu.bht[i].target_pc = ex_rsp->target_pc;
            return;
        }
    }

    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (!ifu->bpu.bht[i].vld) {
            ifu->bpu.bht[i].vld = true;
            ifu->bpu.bht[i].pc = ex_rsp->pc;
            ifu->bpu.bht[i].taken = ex_rsp->taken;
            ifu->bpu.bht[i].target_pc = ex_rsp->target_pc;
            return;
        }
    }

    u32 lru_idx = 0;
    u32 lru_hit_cnt = calc_used_times(ifu->bpu.bht[lru_idx].used_bits);
    for (u32 i = 0; i < IFU_BHT_SIZE; i++) {
        if (!ifu->bpu.bht[i].vld) {
            continue;
        }

        u32 hit_cnt = calc_used_times(ifu->bpu.bht[i].used_bits);
        if (hit_cnt < lru_hit_cnt) {
            lru_idx = i;
            lru_hit_cnt = hit_cnt;
        }
    }

    DBG_CHECK(ifu->bpu.bht[lru_idx].vld);
    ifu->bpu.bht[lru_idx].pc = ex_rsp->pc;
    ifu->bpu.bht[lru_idx].taken = ex_rsp->taken;
    ifu->bpu.bht[lru_idx].target_pc = ex_rsp->target_pc;
    ifu->bpu.bht[lru_idx].used_bits = 0u;
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
    (*ifu->perf.branch)++;

    if (ex_rsp.pred_true) {
        (*ifu->perf.pred_true)++;
        return;
    }

    fl_req_if_t fl_req;
    itf_write(ifu->fl_req_mst, &fl_req);

    ifu_update_bpu_bht(ifu, &ex_rsp);

    ifu->resume.pc = ex_rsp.target_pc;
    ifu->resume.vld = true;
}

static void ifu_proc_fl_rsp(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->fl_rsp_slv)) {
        return;
    }

    fl_rsp_if_t fl_rsp;
    itf_read(ifu->fl_rsp_slv, &fl_rsp);
}

void ifu_clock(ifu_t *ifu)
{
    ifu_send_fch(ifu);
    ifu_proc_fch_rsp(ifu);
    ifu_prepare_issue(ifu);
    ifu_send_ex_req(ifu);
    ifu_proc_ex_rsp(ifu);
    ifu_proc_fl_rsp(ifu);
}