#include "bpu.h"
#include "spec/core/isa.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "dbg/pcm.h"

#define BPU_RAS_LINK_RA 1u
#define BPU_RAS_LINK_T0 5u

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

static bool bpu_is_link_reg(u32 reg)
{
    return reg == BPU_RAS_LINK_RA || reg == BPU_RAS_LINK_T0;
}

static bool bpu_jalr_reads_ras(const rv32g_inst_t *inst)
{
    bool rd_link = bpu_is_link_reg(inst->i.rd);
    bool rs1_link = bpu_is_link_reg(inst->i.rs1);

    return rs1_link && (!rd_link || inst->i.rd != inst->i.rs1);
}

static u32 bpu_cond_bht_idx(u32 pc)
{
    return (pc >> 2) & (BPU_COND_BHT_SIZE - 1u);
}

static void bpu_ras_push(bpu_ras_t *ras, u32 pc)
{
    if (BPU_RAS_SIZE == 0) {
        return;
    }

    ras->entry[ras->sp] = pc;
    ras->sp = (ras->sp + 1u) % BPU_RAS_SIZE;
    if (ras->count < BPU_RAS_SIZE) {
        ras->count++;
    }
}

static bool bpu_ras_peek(const bpu_ras_t *ras, u32 *pc)
{
    if (ras->count == 0) {
        return false;
    }

    u32 idx = (ras->sp + BPU_RAS_SIZE - 1u) % BPU_RAS_SIZE;
    *pc = ras->entry[idx];
    return true;
}

static void bpu_ras_pop(bpu_ras_t *ras)
{
    if (ras->count == 0) {
        return;
    }

    ras->sp = (ras->sp + BPU_RAS_SIZE - 1u) % BPU_RAS_SIZE;
    ras->count--;
}

static bool bpu_jalr_btb_lookup(const bpu_t *bpu, u32 pc, u32 *target_pc)
{
    for (u32 i = 0; i < BPU_JALR_BTB_SIZE; i++) {
        if (!bpu->jalr_btb[i].vld) {
            continue;
        }

        if (bpu->jalr_btb[i].pc == pc) {
            *target_pc = bpu->jalr_btb[i].target_pc;
            return true;
        }
    }

    return false;
}

static void bpu_jalr_btb_touch(bpu_t *bpu, u32 pc)
{
    bpu->access_seq++;
    for (u32 i = 0; i < BPU_JALR_BTB_SIZE; i++) {
        if (bpu->jalr_btb[i].vld && bpu->jalr_btb[i].pc == pc) {
            bpu->jalr_btb[i].last_used = bpu->access_seq;
            return;
        }
    }
}

static void bpu_update_jalr_btb(bpu_t *bpu, u32 pc, u32 target_pc)
{
    bpu->access_seq++;
    for (u32 i = 0; i < BPU_JALR_BTB_SIZE; i++) {
        if (bpu->jalr_btb[i].vld && bpu->jalr_btb[i].pc == pc) {
            bpu->jalr_btb[i].target_pc = target_pc;
            bpu->jalr_btb[i].last_used = bpu->access_seq;
            return;
        }
    }

    for (u32 i = 0; i < BPU_JALR_BTB_SIZE; i++) {
        if (!bpu->jalr_btb[i].vld) {
            bpu->jalr_btb[i].vld = true;
            bpu->jalr_btb[i].pc = pc;
            bpu->jalr_btb[i].target_pc = target_pc;
            bpu->jalr_btb[i].last_used = bpu->access_seq;
            return;
        }
    }

    u32 lru_idx = 0;
    u32 lru_min = bpu->jalr_btb[0].last_used;
    for (u32 i = 1; i < BPU_JALR_BTB_SIZE; i++) {
        if (bpu->jalr_btb[i].last_used < lru_min) {
            lru_idx = i;
            lru_min = bpu->jalr_btb[i].last_used;
        }
    }

    bpu->jalr_btb[lru_idx].vld = true;
    bpu->jalr_btb[lru_idx].pc = pc;
    bpu->jalr_btb[lru_idx].target_pc = target_pc;
    bpu->jalr_btb[lru_idx].last_used = bpu->access_seq;
}

static void bpu_update_ras_state(bpu_ras_t *ras, const bpu_update_if_t *update)
{
    rv32g_inst_t inst = { .raw = update->ir };
    if (inst.base.opcode == OPCODE_JAL) {
        if (bpu_is_link_reg(inst.j.rd)) {
            bpu_ras_push(ras, update->pc + 4u);
        }
        return;
    }

    if (inst.base.opcode != OPCODE_JALR) {
        return;
    }

    bool rd_link = bpu_is_link_reg(inst.i.rd);
    bool rs1_link = bpu_is_link_reg(inst.i.rs1);

    if (!rd_link && rs1_link) {
        bpu_ras_pop(ras);
    } else if (rd_link && !rs1_link) {
        bpu_ras_push(ras, update->pc + 4u);
    } else if (rd_link && rs1_link) {
        if (inst.i.rd != inst.i.rs1) {
            bpu_ras_pop(ras);
        }
        bpu_ras_push(ras, update->pc + 4u);
    }
}

static void bpu_update_ras(bpu_t *bpu, const bpu_update_if_t *update)
{
    bpu_update_ras_state(&bpu->ras, update);
}

static void bpu_update_tables(bpu_t *bpu, const bpu_update_if_t *update)
{
    rv32g_inst_t inst = { .raw = update->ir };
    if (inst.base.opcode == OPCODE_BRANCH) {
        u32 idx = bpu_cond_bht_idx(update->pc);
        if (!bpu->cond_bht[idx].vld) {
            bpu->cond_bht[idx].vld = true;
            bpu->cond_bht[idx].counter = update->taken ? 2 : 1;
        } else {
            sat_counter_update(&bpu->cond_bht[idx].counter, update->taken);
        }
        return;
    }

    if (inst.base.opcode == OPCODE_JALR) {
        bpu_update_jalr_btb(bpu, update->pc, update->target_pc);
    }
}

static void bpu_update_pred_meta(bpu_t *bpu, const bpu_update_if_t *update)
{
    if (update->pred_cond_bht_hit) {
        (*bpu->perf.cond_bht_hit)++;
    }

    if (update->pred_jalr_ras_hit) {
        (*bpu->perf.ras_pred)++;
        (*bpu->perf.jalr_ras_hit)++;
    }

    if (update->pred_jalr_btb_hit) {
        (*bpu->perf.jalr_btb_hit)++;
        bpu_jalr_btb_touch(bpu, update->pc);
    }

    if (update->pred_jalr_btb_miss) {
        (*bpu->perf.jalr_btb_miss)++;
    }
}

static bool bpu_update_is_opcode(const bpu_update_if_t *update, u32 opcode)
{
    if (!update->vld) {
        return false;
    }

    rv32g_inst_t inst = { .raw = update->ir };
    return inst.base.opcode == opcode;
}

static bool bpu_cond_bht_lookup_effective(const bpu_t *bpu,
    const bpu_update_if_t *update, u32 pc, u8 *counter)
{
    u32 idx = bpu_cond_bht_idx(pc);
    bool vld = bpu->cond_bht[idx].vld;
    u8 cnt = bpu->cond_bht[idx].counter;

    if (bpu_update_is_opcode(update, OPCODE_BRANCH) &&
        bpu_cond_bht_idx(update->pc) == idx) {
        if (!vld) {
            vld = true;
            cnt = update->taken ? 2 : 1;
        } else {
            sat_counter_update(&cnt, update->taken);
        }
    }

    if (!vld) {
        return false;
    }

    *counter = cnt;
    return true;
}

static bool bpu_jalr_btb_lookup_effective(const bpu_t *bpu,
    const bpu_update_if_t *update, u32 pc, u32 *target_pc)
{
    if (bpu_update_is_opcode(update, OPCODE_JALR) && update->pc == pc) {
        *target_pc = update->target_pc;
        return true;
    }

    return bpu_jalr_btb_lookup(bpu, pc, target_pc);
}

static void bpu_ras_apply_effective_update(bpu_ras_t *ras,
    const bpu_update_if_t *update)
{
    if (!update->vld) {
        return;
    }

    bpu_update_ras_state(ras, update);
}

static void bpu_pred_req_cb(void *args);

void bpu_construct(bpu_t *bpu, const char *parent, const char *name)
{
    mod_construct(&bpu->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(bpu->pred_req_slv != NULL);
    DBG_CHECK(bpu->pred_rsp_mst != NULL);
    DBG_CHECK(bpu->update_slv != NULL);
    DBG_CHECK(bpu->pred_req_slv->mode == ITF_MODE_SIGNAL);
    DBG_CHECK(bpu->pred_rsp_mst->mode == ITF_MODE_SIGNAL);
    DBG_CHECK(bpu->update_slv->mode == ITF_MODE_SIGNAL);
    DBG_CHECK(BPU_COND_BHT_SIZE > 0);
    DBG_CHECK((BPU_COND_BHT_SIZE & (BPU_COND_BHT_SIZE - 1u)) == 0);
    DBG_CHECK(BPU_JALR_BTB_SIZE > 0);
    DBG_CHECK(BPU_RAS_SIZE > 0);

    bpu->perf.branch = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name, "branch");
    bpu->perf.pred_true = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "pred_true");
    bpu->perf.cond_branch = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "cond_branch");
    bpu->perf.cond_branch_pred_true = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "cond_branch_pred_true");
    bpu->perf.jal = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name, "jal");
    bpu->perf.jal_pred_true = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "jal_pred_true");
    bpu->perf.jalr = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name, "jalr");
    bpu->perf.jalr_pred_true = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "jalr_pred_true");
    bpu->perf.cond_bht_hit = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "cond_bht_hit");
    bpu->perf.ras_pred = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name, "ras_pred");
    bpu->perf.jalr_ras_hit = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "jalr_ras_hit");
    bpu->perf.jalr_btb_hit = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "jalr_btb_hit");
    bpu->perf.jalr_btb_miss = dbg_pcm_reg_perf_cnt(bpu->mod.hier_name,
        "jalr_btb_miss");

    dbg_vcd_add_sig("ras_count", DBG_SIG_TYPE_REG, 32, &bpu->ras.count);

    bpu->pred_req_i = itf_signal_get_src_and_chk(bpu->pred_req_slv);
    itf_signal_set_wcb(bpu->pred_req_slv, &bpu_pred_req_cb, bpu);
}

void bpu_reset(bpu_t *bpu)
{
    mod_reset(&bpu->mod);

    bpu->access_seq = 0;
    for (u32 i = 0; i < BPU_COND_BHT_SIZE; i++) {
        bpu->cond_bht[i].vld = false;
        bpu->cond_bht[i].counter = 0;
    }
    for (u32 i = 0; i < BPU_JALR_BTB_SIZE; i++) {
        bpu->jalr_btb[i].vld = false;
        bpu->jalr_btb[i].pc = 0;
        bpu->jalr_btb[i].target_pc = 0;
        bpu->jalr_btb[i].last_used = 0;
    }
    bpu->ras.sp = 0;
    bpu->ras.count = 0;
    for (u32 i = 0; i < BPU_RAS_SIZE; i++) {
        bpu->ras.entry[i] = 0;
    }

    *bpu->perf.branch = 0;
    *bpu->perf.pred_true = 0;
    *bpu->perf.cond_branch = 0;
    *bpu->perf.cond_branch_pred_true = 0;
    *bpu->perf.jal = 0;
    *bpu->perf.jal_pred_true = 0;
    *bpu->perf.jalr = 0;
    *bpu->perf.jalr_pred_true = 0;
    *bpu->perf.cond_bht_hit = 0;
    *bpu->perf.ras_pred = 0;
    *bpu->perf.jalr_ras_hit = 0;
    *bpu->perf.jalr_btb_hit = 0;
    *bpu->perf.jalr_btb_miss = 0;
}

static void bpu_eval_pred_req(bpu_t *bpu)
{
    const bpu_pred_req_if_t *req = bpu->pred_req_i;
    bpu_update_if_t update;
    itf_read(bpu->update_slv, &update);

    bpu_pred_rsp_if_t rsp = {
        .vld = req->vld,
        .is_ctrl = false,
        .pred_taken = false,
        .pred_pc = req->pc + 4u
    };

    if (!req->vld) {
        itf_write(bpu->pred_rsp_mst, &rsp);
        return;
    }

    rv32g_inst_t inst = { .raw = req->ir };
    rsp.is_ctrl =
        inst.base.opcode == OPCODE_JAL ||
        inst.base.opcode == OPCODE_JALR ||
        inst.base.opcode == OPCODE_BRANCH;

    if (!rsp.is_ctrl) {
        itf_write(bpu->pred_rsp_mst, &rsp);
        return;
    }

    if (inst.base.opcode == OPCODE_JAL) {
        rsp.pred_taken = true;
        rsp.pred_pc = req->pc + j_imm_decode(&inst).u;
        itf_write(bpu->pred_rsp_mst, &rsp);
        return;
    }

    if (inst.base.opcode == OPCODE_BRANCH) {
        i32 offset = b_imm_decode(&inst);
        u8 counter;

        if (bpu_cond_bht_lookup_effective(bpu, &update, req->pc, &counter)) {
            rsp.cond_bht_hit = true;
            rsp.pred_taken = counter >= 2;
            if (rsp.pred_taken) {
                rsp.pred_pc = req->pc + offset.u;
            }
            itf_write(bpu->pred_rsp_mst, &rsp);
            return;
        }

        if (offset.s < 0) {
            rsp.pred_taken = true;
            rsp.pred_pc = req->pc + offset.u;
        }
        itf_write(bpu->pred_rsp_mst, &rsp);
        return;
    }

    if (inst.base.opcode == OPCODE_JALR && bpu_jalr_reads_ras(&inst)) {
        bpu_ras_t ras = bpu->ras;
        bpu_ras_apply_effective_update(&ras, &update);
        u32 target_pc;
        if (bpu_ras_peek(&ras, &target_pc)) {
            rsp.pred_taken = true;
            rsp.pred_pc = target_pc;
            rsp.jalr_ras_hit = true;
            itf_write(bpu->pred_rsp_mst, &rsp);
            return;
        }
    }

    if (inst.base.opcode == OPCODE_JALR) {
        u32 target_pc;
        if (bpu_jalr_btb_lookup_effective(bpu, &update, req->pc, &target_pc)) {
            rsp.pred_taken = true;
            rsp.pred_pc = target_pc;
            rsp.jalr_btb_hit = true;
        } else {
            rsp.jalr_btb_miss = true;
        }
    }

    itf_write(bpu->pred_rsp_mst, &rsp);
}

static void bpu_pred_req_cb(void *args)
{
    bpu_t *bpu = args;
    bpu_eval_pred_req(bpu);
}

void bpu_clock(bpu_t *bpu)
{
    mod_clock(&bpu->mod);

    bpu_update_if_t update;
    itf_read(bpu->update_slv, &update);
    if (!update.vld) {
        return;
    }

    rv32g_inst_t inst = { .raw = update.ir };

    if (!update.is_boot_code) {
        (*bpu->perf.branch)++;
        if (inst.base.opcode == OPCODE_BRANCH) {
            (*bpu->perf.cond_branch)++;
        } else if (inst.base.opcode == OPCODE_JAL) {
            (*bpu->perf.jal)++;
        } else if (inst.base.opcode == OPCODE_JALR) {
            (*bpu->perf.jalr)++;
        }
    }

    bpu_update_pred_meta(bpu, &update);
    bpu_update_tables(bpu, &update);
    bpu_update_ras(bpu, &update);

    if (!update.pred_true) {
        return;
    }

    if (!update.is_boot_code) {
        (*bpu->perf.pred_true)++;
        if (inst.base.opcode == OPCODE_BRANCH) {
            (*bpu->perf.cond_branch_pred_true)++;
        } else if (inst.base.opcode == OPCODE_JAL) {
            (*bpu->perf.jal_pred_true)++;
        } else if (inst.base.opcode == OPCODE_JALR) {
            (*bpu->perf.jalr_pred_true)++;
        }
    }
}

void bpu_free(bpu_t *bpu)
{
    mod_free(&bpu->mod);
}
