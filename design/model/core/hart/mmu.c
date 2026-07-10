#include "mmu.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

#define MMU_PGSHIFT 12u
#define MMU_PGSIZE (1u << MMU_PGSHIFT)
#define MMU_PGMASK (MMU_PGSIZE - 1u)
#define MMU_PTE_SIZE 4u
#define MMU_PTE_TRANS_ID 0xfffeu
#define MMU_SATP_MODE_SV32 1u

#define PTE_V (1u << 0)
#define PTE_R (1u << 1)
#define PTE_W (1u << 2)
#define PTE_X (1u << 3)
#define PTE_U (1u << 4)
#define PTE_A (1u << 6)
#define PTE_D (1u << 7)

typedef enum mmu_access {
    MMU_ACCESS_INST,
    MMU_ACCESS_LOAD,
    MMU_ACCESS_STORE,
} mmu_access_t;

static bool mmu_pte_permits(const mmu_t *mmu, u32 pte);
static bool mmu_pte_ad_ok(const mmu_t *mmu, u32 pte);

static rv32g_csr_mstatus_t mmu_mstatus(const mmu_t *mmu)
{
    return (rv32g_csr_mstatus_t){ .raw = mmu->csr_state_i->mstatus };
}

static rv32g_csr_satp_t mmu_satp(const mmu_t *mmu)
{
    return (rv32g_csr_satp_t){ .raw = mmu->csr_state_i->satp };
}

void mmu_construct(mmu_t *mmu, const char *parent, const char *name, const mmu_conf_t *conf)
{
    mod_construct(&mmu->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->i_stg_fifo_depth > 0);
    DBG_CHECK(conf->d_stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);
    mmu->exu_state_i = itf_signal_get_src_and_chk(mmu->exu_state_in);
    mmu->csr_state_i = itf_signal_get_src_and_chk(mmu->csr_mmu_state_in);
    fifo_construct(&mmu->va_i_req_fifo, sizeof(bti_req_if_t),
        conf->i_stg_fifo_depth);
    fifo_construct(&mmu->va_d_req_fifo, sizeof(bti_req_if_t),
        conf->d_stg_fifo_depth);
    ostq_construct(&mmu->i_ost, sizeof(mmu_ost_ctx_t), conf->ost_depth);
    ostq_construct(&mmu->d_ost, sizeof(mmu_ost_ctx_t), conf->ost_depth);

    dbg_vcd_add_sig("busy", DBG_SIG_TYPE_REG, 1, &mmu->busy);
    dbg_vcd_add_sig("is_inst", DBG_SIG_TYPE_REG, 1, &mmu->is_inst);
    dbg_vcd_add_sig("req_addr", DBG_SIG_TYPE_REG, 32, &mmu->req.addr);
    dbg_vcd_add_sig("req_cmd", DBG_SIG_TYPE_REG, 1, &mmu->req.cmd);
    dbg_vcd_add_sig("req_priv", DBG_SIG_TYPE_REG, 2, &mmu->req_priv);
    dbg_vcd_add_sig("req_sum", DBG_SIG_TYPE_REG, 1, &mmu->req_sum);
    dbg_vcd_add_sig("req_mxr", DBG_SIG_TYPE_REG, 1, &mmu->req_mxr);
    dbg_vcd_add_sig("fault_pc", DBG_SIG_TYPE_REG, 32, &mmu->fault_pc);
    dbg_vcd_add_sig("va", DBG_SIG_TYPE_REG, 32, &mmu->va);
    dbg_vcd_add_sig("pte", DBG_SIG_TYPE_REG, 32, &mmu->pte);
    dbg_vcd_add_sig("leaf_pa_base", DBG_SIG_TYPE_REG, 32, &mmu->leaf_pa_base);
    dbg_vcd_add_sig("level", DBG_SIG_TYPE_REG, 1, &mmu->level);
    dbg_vcd_add_sig("pte_req_pending", DBG_SIG_TYPE_REG, 1, &mmu->pte_req_pending);
    dbg_vcd_add_sig("final_req_pending", DBG_SIG_TYPE_REG, 1, &mmu->final_req_pending);
    dbg_vcd_add_sig("final_rsp_pending", DBG_SIG_TYPE_REG, 1, &mmu->final_rsp_pending);
    dbg_vcd_add_sig("va_i_req_fifo_num", DBG_SIG_TYPE_REG, 32,
        &mmu->va_i_req_fifo.num);
    dbg_vcd_add_sig("va_d_req_fifo_num", DBG_SIG_TYPE_REG, 32,
        &mmu->va_d_req_fifo.num);
    dbg_vcd_add_sig("i_ost_count", DBG_SIG_TYPE_REG, 32, &mmu->i_ost.count);
    dbg_vcd_add_sig("d_ost_count", DBG_SIG_TYPE_REG, 32, &mmu->d_ost.count);
    dbg_vcd_add_sig("itlb_replace_idx", DBG_SIG_TYPE_REG, 4, &mmu->itlb_replace_idx);
    dbg_vcd_add_sig("dtlb_replace_idx", DBG_SIG_TYPE_REG, 4, &mmu->dtlb_replace_idx);

    mmu->perf_itlb_hit = dbg_pcm_reg_perf_cnt(mmu->mod.hier_name, "itlb_hit");
    mmu->perf_itlb_miss = dbg_pcm_reg_perf_cnt(mmu->mod.hier_name, "itlb_miss");
    mmu->perf_dtlb_hit = dbg_pcm_reg_perf_cnt(mmu->mod.hier_name, "dtlb_hit");
    mmu->perf_dtlb_miss = dbg_pcm_reg_perf_cnt(mmu->mod.hier_name, "dtlb_miss");
    mmu->perf_i_stg_full = dbg_pcm_reg_perf_cnt(mmu->mod.hier_name,
        "i_stg_full");
    mmu->perf_d_stg_full = dbg_pcm_reg_perf_cnt(mmu->mod.hier_name,
        "d_stg_full");
}

void mmu_reset(mmu_t *mmu)
{
    mod_reset(&mmu->mod);
    mmu->busy = false;
    mmu->pte_req_pending = false;
    mmu->final_req_pending = false;
    mmu->final_rsp_pending = false;
    mmu->drop_ptw_rsp = false;
    mmu->drop_i_rsp = false;
    mmu->bare_i_rsp_pending = false;
    mmu->walk_slot_vld = false;
    fifo_reset(&mmu->va_i_req_fifo);
    fifo_reset(&mmu->va_d_req_fifo);
    ostq_reset(&mmu->i_ost);
    ostq_reset(&mmu->d_ost);
    mmu->itlb_replace_idx = 0;
    mmu->dtlb_replace_idx = 0;
    for (u32 i = 0; i < MMU_ITLB_SIZE; i++) {
        mmu->itlb[i].vld = false;
    }
    for (u32 i = 0; i < MMU_DTLB_SIZE; i++) {
        mmu->dtlb[i].vld = false;
    }
    *mmu->perf_itlb_hit = 0;
    *mmu->perf_itlb_miss = 0;
    *mmu->perf_dtlb_hit = 0;
    *mmu->perf_dtlb_miss = 0;
    *mmu->perf_i_stg_full = 0;
    *mmu->perf_d_stg_full = 0;
}

static rv32g_priv_t mmu_effective_priv(const mmu_t *mmu, mmu_access_t access)
{
    rv32g_csr_mstatus_t mstatus = mmu_mstatus(mmu);
    rv32g_priv_t priv = (rv32g_priv_t)mmu->exu_state_i->priv;
    if (access != MMU_ACCESS_INST && priv == RV32G_PRIV_MACHINE &&
        mstatus.reg.mprv) {
        return (rv32g_priv_t)mstatus.reg.mpp;
    }
    return priv;
}

static bool mmu_translation_enabled(const mmu_t *mmu, mmu_access_t access)
{
    rv32g_priv_t priv = mmu_effective_priv(mmu, access);
    return priv != RV32G_PRIV_MACHINE &&
        mmu_satp(mmu).reg.mode == MMU_SATP_MODE_SV32;
}

static ostq_t *mmu_ost(mmu_t *mmu, bool is_inst)
{
    return is_inst ? &mmu->i_ost : &mmu->d_ost;
}

static void mmu_complete_normal(mmu_t *mmu, bool is_inst, u32 slot,
    const bti_rsp_if_t *rsp)
{
    ostq_t *ost = mmu_ost(mmu, is_inst);
    mmu_ost_ctx_t ctx;
    ostq_read_slot(ost, slot, &ctx);
    ctx.rsp = *rsp;
    ctx.ready = true;
    ctx.expt_vld = false;
    ostq_write_slot(ost, slot, &ctx);
}

static void mmu_complete_fault(mmu_t *mmu, bool is_inst, u32 slot,
    hart_expt_cause_t cause, u8 priv, u32 pc, u32 tval)
{
    ostq_t *ost = mmu_ost(mmu, is_inst);
    mmu_ost_ctx_t ctx;
    ostq_read_slot(ost, slot, &ctx);
    ctx.expt.type = HART_EXPT_TYPE_EXCEPTION;
    ctx.expt.cause = cause;
    ctx.expt.priv = priv;
    ctx.expt.pc = pc;
    ctx.expt.tval = tval;
    ctx.expt_vld = true;
    ctx.rsp.trans_id = ctx.req.trans_id;
    ctx.rsp.data = 0;
    ctx.rsp.ok = false;
    ctx.ready = true;
    ostq_write_slot(ost, slot, &ctx);
}

static void mmu_complete_current_fault(mmu_t *mmu, hart_expt_cause_t cause)
{
    DBG_CHECK(mmu->walk_slot_vld);
    mmu_complete_fault(mmu, mmu->is_inst, mmu->walk_slot, cause,
        mmu->req_priv, mmu->fault_pc, mmu->va);
}

static void mmu_raise_fault(mmu_t *mmu)
{
    hart_expt_cause_t cause;
    if (mmu->is_inst) {
        cause = HART_EXPT_CAUSE_INST_PAGE_FAULT;
    } else if (mmu->req.cmd == BTI_REQ_CMD_WRITE) {
        cause = HART_EXPT_CAUSE_STORE_AMO_PAGE_FAULT;
    } else {
        cause = HART_EXPT_CAUSE_LOAD_PAGE_FAULT;
    }

    mmu_complete_current_fault(mmu, cause);
    mmu->busy = false;
    mmu->walk_slot_vld = false;
    mmu->pte_req_pending = false;
    mmu->final_req_pending = false;
    mmu->final_rsp_pending = false;
}

static bool mmu_find_pa_rsp_slot(mmu_t *mmu, bool is_inst, mmu_ost_ctx_t *ctx,
    u32 *slot)
{
    ostq_t *ost = mmu_ost(mmu, is_inst);
    for (u32 i = 0; i < ost->depth; i++) {
        u32 idx = (ost->rptr + i) % ost->depth;
        if (!ostq_slot_valid(ost, idx)) {
            continue;
        }

        mmu_ost_ctx_t cur;
        ostq_read_slot(ost, idx, &cur);
        if (!cur.pa_issued || cur.ready) {
            continue;
        }

        if (ctx != NULL) {
            *ctx = cur;
        }
        if (slot != NULL) {
            *slot = idx;
        }
        return true;
    }
    return false;
}

static void mmu_send_pa_req(mmu_t *mmu, itf_t *pa_req_mst, const bti_req_if_t *req)
{
    DBG_CHECK(!itf_fifo_full(pa_req_mst));
    itf_write(pa_req_mst, req);
}

static void mmu_start_req(mmu_t *mmu, bool is_inst, const bti_req_if_t *req,
    u32 slot)
{
    mmu_access_t access = is_inst ? MMU_ACCESS_INST :
        (req->cmd == BTI_REQ_CMD_WRITE ? MMU_ACCESS_STORE : MMU_ACCESS_LOAD);

    mmu->busy = true;
    mmu->is_inst = is_inst;
    mmu->req = *req;
    mmu->req_priv = mmu_effective_priv(mmu, access);
    mmu->req_sum = mmu_mstatus(mmu).reg.sum;
    mmu->req_mxr = mmu_mstatus(mmu).reg.mxr;
    mmu->fault_pc = is_inst ? req->addr : mmu->exu_state_i->pc;
    mmu->va = req->addr;
    mmu->root_base = mmu_satp(mmu).reg.ppn << MMU_PGSHIFT;
    mmu->level = 1;
    mmu->pte_req_pending = false;
    mmu->final_req_pending = false;
    mmu->walk_slot_vld = true;
    mmu->walk_is_inst = is_inst;
    mmu->walk_slot = slot;
}

static void mmu_invalidate_tlb(mmu_t *mmu)
{
    for (u32 i = 0; i < MMU_ITLB_SIZE; i++) {
        mmu->itlb[i].vld = false;
    }
    for (u32 i = 0; i < MMU_DTLB_SIZE; i++) {
        mmu->dtlb[i].vld = false;
    }
}

static void mmu_recv_tlb_flush(mmu_t *mmu)
{
    if (itf_fifo_empty(mmu->tlb_flush_slv)) {
        return;
    }

    tlb_flush_if_t pkt;
    itf_read(mmu->tlb_flush_slv, &pkt);
    mmu_invalidate_tlb(mmu);
}

static void mmu_drop_canceled_rsp(mmu_t *mmu)
{
    if (mmu->drop_ptw_rsp && !itf_fifo_empty(mmu->ptw_bti_rsp_slv)) {
        bti_rsp_if_t rsp;
        itf_read(mmu->ptw_bti_rsp_slv, &rsp);
        DBG_CHECK(rsp.trans_id == MMU_PTE_TRANS_ID);
        mmu->drop_ptw_rsp = false;
    }

    if (mmu->drop_i_rsp && !itf_fifo_empty(mmu->pa_i_bti_rsp_slv)) {
        bti_rsp_if_t rsp;
        itf_read(mmu->pa_i_bti_rsp_slv, &rsp);
        mmu->drop_i_rsp = false;
    }
}

static u32 mmu_tlb_tag(u32 va, int level)
{
    return level == 1 ? (va >> 22u) : (va >> 12u);
}

static u32 mmu_tlb_page_mask(int level)
{
    return level == 1 ? ((1u << 22u) - 1u) : MMU_PGMASK;
}

static mmu_tlb_entry_t *mmu_tlb_entries(mmu_t *mmu, bool is_inst, u32 *num)
{
    if (is_inst) {
        *num = MMU_ITLB_SIZE;
        return mmu->itlb;
    }

    *num = MMU_DTLB_SIZE;
    return mmu->dtlb;
}

static void mmu_tlb_fill(mmu_t *mmu)
{
    u32 num;
    mmu_tlb_entry_t *entries = mmu_tlb_entries(mmu, mmu->is_inst, &num);
    u32 *replace_idx = mmu->is_inst ? &mmu->itlb_replace_idx : &mmu->dtlb_replace_idx;
    mmu_tlb_entry_t *entry = entries + *replace_idx;

    entry->vld = true;
    entry->tag = mmu_tlb_tag(mmu->va, mmu->level);
    rv32g_csr_satp_t satp = mmu_satp(mmu);
    entry->satp_ppn = satp.reg.ppn;
    entry->satp_asid = satp.reg.asid;
    entry->priv = mmu->req_priv;
    entry->sum = mmu->req_sum;
    entry->mxr = mmu->req_mxr;
    entry->level = mmu->level;
    entry->pte = mmu->pte;
    entry->pa_base = mmu->leaf_pa_base & ~mmu_tlb_page_mask(mmu->level);

    *replace_idx = (*replace_idx + 1u) % num;
}

static void mmu_capture_req(mmu_t *mmu)
{
    if (fifo_full(&mmu->va_i_req_fifo)) {
        if (!itf_fifo_empty(mmu->va_i_bti_req_slv)) {
            (*mmu->perf_i_stg_full)++;
        }
    } else if (!itf_fifo_empty(mmu->va_i_bti_req_slv)) {
        bti_req_if_t req;
        itf_read(mmu->va_i_bti_req_slv, &req);
        fifo_push(&mmu->va_i_req_fifo, &req);
    }

    if (fifo_full(&mmu->va_d_req_fifo)) {
        if (!itf_fifo_empty(mmu->va_d_bti_req_slv)) {
            (*mmu->perf_d_stg_full)++;
        }
    } else if (!itf_fifo_empty(mmu->va_d_bti_req_slv)) {
        bti_req_if_t req;
        itf_read(mmu->va_d_bti_req_slv, &req);
        fifo_push(&mmu->va_d_req_fifo, &req);
    }
}

static bool mmu_alloc_ost_req(mmu_t *mmu, bool is_inst, const bti_req_if_t *req,
    u32 *slot)
{
    ostq_t *ost = mmu_ost(mmu, is_inst);
    if (ostq_full(ost)) {
        return false;
    }

    mmu_ost_ctx_t ctx = {
        .is_inst = is_inst,
        .req = *req,
        .pa_issued = false,
        .ready = false,
        .expt_vld = false
    };
    return ostq_alloc(ost, &ctx, slot);
}

static bool mmu_lookup_tlb_entry(mmu_t *mmu, bool is_inst,
    const bti_req_if_t *req, mmu_tlb_entry_t **entry_out)
{
    u32 num;
    mmu_tlb_entry_t *entries = mmu_tlb_entries(mmu, is_inst, &num);
    mmu_access_t access = is_inst ? MMU_ACCESS_INST :
        (req->cmd == BTI_REQ_CMD_WRITE ? MMU_ACCESS_STORE : MMU_ACCESS_LOAD);
    rv32g_priv_t req_priv = mmu_effective_priv(mmu, access);
    bool req_sum = mmu_mstatus(mmu).reg.sum;
    bool req_mxr = mmu_mstatus(mmu).reg.mxr;
    rv32g_csr_satp_t satp = mmu_satp(mmu);

    for (u32 i = 0; i < num; i++) {
        mmu_tlb_entry_t *entry = entries + i;
        if (!entry->vld) {
            continue;
        }
        if (entry->satp_ppn != satp.reg.ppn ||
            entry->satp_asid != satp.reg.asid ||
            entry->priv != req_priv ||
            entry->sum != req_sum ||
            entry->mxr != req_mxr ||
            entry->tag != mmu_tlb_tag(req->addr, entry->level)) {
            continue;
        }
        *entry_out = entry;
        return true;
    }
    return false;
}

static void mmu_accept_tlb_hit(mmu_t *mmu, bool is_inst,
    const bti_req_if_t *req, u32 slot, mmu_tlb_entry_t *entry)
{
    mmu_access_t access = is_inst ? MMU_ACCESS_INST :
        (req->cmd == BTI_REQ_CMD_WRITE ? MMU_ACCESS_STORE : MMU_ACCESS_LOAD);
    rv32g_priv_t req_priv = mmu_effective_priv(mmu, access);
    bool req_sum = mmu_mstatus(mmu).reg.sum;
    bool req_mxr = mmu_mstatus(mmu).reg.mxr;

    mmu->busy = true;
    mmu->is_inst = is_inst;
    mmu->req = *req;
    mmu->req_priv = req_priv;
    mmu->req_sum = req_sum;
    mmu->req_mxr = req_mxr;
    mmu->fault_pc = is_inst ? req->addr : mmu->exu_state_i->pc;
    mmu->va = req->addr;
    mmu->pte = entry->pte;
    if (!mmu_pte_permits(mmu, entry->pte) || !mmu_pte_ad_ok(mmu, entry->pte)) {
        hart_expt_cause_t cause = is_inst ?
            HART_EXPT_CAUSE_INST_PAGE_FAULT :
            (req->cmd == BTI_REQ_CMD_WRITE ?
                HART_EXPT_CAUSE_STORE_AMO_PAGE_FAULT :
                HART_EXPT_CAUSE_LOAD_PAGE_FAULT);
        mmu_complete_fault(mmu, is_inst, slot, cause, req_priv,
            mmu->fault_pc, req->addr);
        mmu->busy = false;
        return;
    }

    bti_req_if_t pa_req = *req;
    pa_req.addr = entry->pa_base | (req->addr & mmu_tlb_page_mask(entry->level));
    mmu_ost_ctx_t ctx;
    ostq_read_slot(mmu_ost(mmu, is_inst), slot, &ctx);
    ctx.pa_issued = true;
    ostq_write_slot(mmu_ost(mmu, is_inst), slot, &ctx);
    mmu_send_pa_req(mmu, is_inst ? mmu->pa_i_bti_req_mst : mmu->pa_d_bti_req_mst,
        &pa_req);
    if (is_inst) {
        (*mmu->perf_itlb_hit)++;
    } else {
        (*mmu->perf_dtlb_hit)++;
    }
    mmu->busy = false;
}

static bool mmu_accept_one_req(mmu_t *mmu, bool is_inst)
{
    fifo_t *fifo = is_inst ? &mmu->va_i_req_fifo : &mmu->va_d_req_fifo;
    if (fifo_empty(fifo)) {
        return false;
    }
    if (mmu->drop_ptw_rsp || mmu->drop_i_rsp) {
        return false;
    }
    if (mmu->busy) {
        return false;
    }

    bti_req_if_t req;
    fifo_get_front(fifo, &req);
    mmu_access_t access = is_inst ? MMU_ACCESS_INST :
        (req.cmd == BTI_REQ_CMD_WRITE ? MMU_ACCESS_STORE : MMU_ACCESS_LOAD);
    itf_t *pa_req_mst = is_inst ? mmu->pa_i_bti_req_mst : mmu->pa_d_bti_req_mst;

    if (!mmu_translation_enabled(mmu, access)) {
        if (itf_fifo_full(pa_req_mst) || ostq_full(mmu_ost(mmu, is_inst))) {
            return false;
        }

        u32 slot;
        bool alloc_ok = mmu_alloc_ost_req(mmu, is_inst, &req, &slot);
        DBG_CHECK(alloc_ok);
        mmu_ost_ctx_t ctx;
        ostq_read_slot(mmu_ost(mmu, is_inst), slot, &ctx);
        ctx.pa_issued = true;
        ostq_write_slot(mmu_ost(mmu, is_inst), slot, &ctx);
        fifo_pop(fifo, &req);
        mmu_send_pa_req(mmu, pa_req_mst, &req);
        return true;
    }

    mmu_tlb_entry_t *entry = NULL;
    if (mmu_lookup_tlb_entry(mmu, is_inst, &req, &entry)) {
        if (itf_fifo_full(pa_req_mst) || ostq_full(mmu_ost(mmu, is_inst))) {
            return false;
        }
        u32 slot;
        bool alloc_ok = mmu_alloc_ost_req(mmu, is_inst, &req, &slot);
        DBG_CHECK(alloc_ok);
        fifo_pop(fifo, &req);
        mmu_accept_tlb_hit(mmu, is_inst, &req, slot, entry);
        return true;
    }

    if (mmu->busy || !ostq_empty(&mmu->i_ost) || !ostq_empty(&mmu->d_ost)) {
        return false;
    }
    if (ostq_full(mmu_ost(mmu, is_inst))) {
        return false;
    }

    u32 slot;
    bool alloc_ok = mmu_alloc_ost_req(mmu, is_inst, &req, &slot);
    DBG_CHECK(alloc_ok);
    fifo_pop(fifo, &req);
    if (is_inst) {
        (*mmu->perf_itlb_miss)++;
    } else {
        (*mmu->perf_dtlb_miss)++;
    }
    mmu_start_req(mmu, is_inst, &req, slot);
    return true;
}

static void mmu_accept_new_req(mmu_t *mmu)
{
    if (mmu_accept_one_req(mmu, false)) {
        return;
    }
    (void)mmu_accept_one_req(mmu, true);
}

static u32 mmu_pte_ppn(u32 pte)
{
    return pte >> 10u;
}

static u32 mmu_pte_ppn0(u32 pte)
{
    return (pte >> 10u) & 0x3ffu;
}

static u32 mmu_pte_ppn1(u32 pte)
{
    return (pte >> 20u) & 0xfffu;
}

static u32 mmu_page_table_index(u32 va, int level)
{
    return level == 1 ? ((va >> 22u) & 0x3ffu) : ((va >> 12u) & 0x3ffu);
}

static u32 mmu_pte_addr(const mmu_t *mmu)
{
    return mmu->root_base + mmu_page_table_index(mmu->va, mmu->level) * MMU_PTE_SIZE;
}

static bool mmu_pte_is_leaf(u32 pte)
{
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

static bool mmu_pte_valid(u32 pte)
{
    if ((pte & PTE_V) == 0) {
        return false;
    }
    if ((pte & PTE_W) && !(pte & PTE_R)) {
        return false;
    }
    return true;
}

static bool mmu_pte_permits(const mmu_t *mmu, u32 pte)
{
    rv32g_priv_t priv = mmu->req_priv;
    bool user_page = (pte & PTE_U) != 0;

    if (priv == RV32G_PRIV_USER && !user_page) {
        return false;
    }
    if (priv == RV32G_PRIV_SUPERVISOR && user_page) {
        if (mmu->is_inst || !mmu->req_sum) {
            return false;
        }
    }

    if (mmu->is_inst) {
        return (pte & PTE_X) != 0;
    }
    if (mmu->req.cmd == BTI_REQ_CMD_WRITE) {
        return (pte & PTE_W) != 0;
    }

    return (pte & PTE_R) || (mmu->req_mxr && (pte & PTE_X));
}

static bool mmu_pte_ad_ok(const mmu_t *mmu, u32 pte)
{
    if ((pte & PTE_A) == 0) {
        return false;
    }
    if (!mmu->is_inst && mmu->req.cmd == BTI_REQ_CMD_WRITE && (pte & PTE_D) == 0) {
        return false;
    }
    return true;
}

static bool mmu_leaf_to_pa(const mmu_t *mmu, u32 pte, u32 *pa)
{
    u32 vpn0 = (mmu->va >> 12u) & 0x3ffu;
    u32 page_off = mmu->va & MMU_PGMASK;

    if (mmu->level == 1) {
        if (mmu_pte_ppn0(pte) != 0) {
            return false;
        }
        *pa = (mmu_pte_ppn1(pte) << 22u) | (vpn0 << 12u) | page_off;
    } else {
        *pa = (mmu_pte_ppn(pte) << 12u) | page_off;
    }
    return true;
}

static void mmu_finish_translate(mmu_t *mmu)
{
    if (!mmu_pte_valid(mmu->pte)) {
        mmu_raise_fault(mmu);
        return;
    }

    if (!mmu_pte_is_leaf(mmu->pte)) {
        if ((mmu->pte & (PTE_U | PTE_A | PTE_D)) != 0 || mmu->level == 0) {
            mmu_raise_fault(mmu);
            return;
        }
        mmu->root_base = mmu_pte_ppn(mmu->pte) << MMU_PGSHIFT;
        mmu->level--;
        return;
    }

    if (!mmu_pte_permits(mmu, mmu->pte) || !mmu_pte_ad_ok(mmu, mmu->pte)) {
        mmu_raise_fault(mmu);
        return;
    }

    u32 pa;
    if (!mmu_leaf_to_pa(mmu, mmu->pte, &pa)) {
        mmu_raise_fault(mmu);
        return;
    }

    mmu->leaf_pa_base = pa;
    mmu_tlb_fill(mmu);
    mmu->req.addr = pa;
    mmu->final_req_pending = true;
}

static void mmu_send_pte_req(mmu_t *mmu)
{
    if (mmu->pte_req_pending || itf_fifo_full(mmu->ptw_bti_req_mst)) {
        return;
    }

    bti_req_if_t req = {};
    req.trans_id = MMU_PTE_TRANS_ID;
    req.cmd = BTI_REQ_CMD_READ;
    req.addr = mmu_pte_addr(mmu);
    req.size = BTI_REQ_SIZE_B4;
    req.data = 0;
    req.strobe = 0xf;
    itf_write(mmu->ptw_bti_req_mst, &req);
    mmu->pte_req_pending = true;
}

static void mmu_recv_pte_rsp(mmu_t *mmu)
{
    if (!mmu->pte_req_pending || itf_fifo_empty(mmu->ptw_bti_rsp_slv)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_read(mmu->ptw_bti_rsp_slv, &rsp);
    DBG_CHECK(rsp.trans_id == MMU_PTE_TRANS_ID);

    mmu->pte_req_pending = false;
    if (!rsp.ok) {
        mmu_raise_fault(mmu);
        return;
    }

    mmu->pte = rsp.data;
    mmu_finish_translate(mmu);
}

static void mmu_send_final_req(mmu_t *mmu)
{
    if (!mmu->final_req_pending) {
        return;
    }

    itf_t *pa_req_mst = mmu->is_inst ? mmu->pa_i_bti_req_mst : mmu->pa_d_bti_req_mst;
    if (itf_fifo_full(pa_req_mst)) {
        return;
    }

    itf_write(pa_req_mst, &mmu->req);
    mmu->final_req_pending = false;
    mmu->final_rsp_pending = true;
}

static void mmu_recv_final_rsp(mmu_t *mmu)
{
    if (!mmu->final_rsp_pending) {
        return;
    }

    itf_t *pa_rsp_slv = mmu->is_inst ? mmu->pa_i_bti_rsp_slv : mmu->pa_d_bti_rsp_slv;
    if (itf_fifo_empty(pa_rsp_slv)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_read(pa_rsp_slv, &rsp);
    DBG_CHECK(mmu->walk_slot_vld);
    mmu_complete_normal(mmu, mmu->is_inst, mmu->walk_slot, &rsp);
    mmu->final_rsp_pending = false;
    mmu->busy = false;
    mmu->walk_slot_vld = false;
}

static void mmu_proc_walk(mmu_t *mmu)
{
    if (!mmu->busy) {
        return;
    }

    if (mmu->final_req_pending) {
        mmu_send_final_req(mmu);
        return;
    }
    if (mmu->final_rsp_pending) {
        mmu_recv_final_rsp(mmu);
        return;
    }

    mmu_recv_pte_rsp(mmu);
    if (mmu->busy && !mmu->final_req_pending) {
        mmu_send_pte_req(mmu);
    }
}

static void mmu_proc_pa_rsp(mmu_t *mmu, bool is_inst)
{
    itf_t *pa_rsp_slv = is_inst ? mmu->pa_i_bti_rsp_slv : mmu->pa_d_bti_rsp_slv;
    if (itf_fifo_empty(pa_rsp_slv)) {
        return;
    }

    mmu_ost_ctx_t ctx;
    u32 slot;
    if (!mmu_find_pa_rsp_slot(mmu, is_inst, &ctx, &slot)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_read(pa_rsp_slv, &rsp);
    mmu_complete_normal(mmu, is_inst, slot, &rsp);
}

static void mmu_send_ready_rsp(mmu_t *mmu, bool is_inst)
{
    ostq_t *ost = mmu_ost(mmu, is_inst);
    mmu_ost_ctx_t ctx;
    bool found = ostq_peek_head(ost, &ctx, NULL);
    if (!found || !ctx.ready) {
        return;
    }

    itf_t *va_rsp_mst = is_inst ? mmu->va_i_bti_rsp_mst : mmu->va_d_bti_rsp_mst;
    itf_t *expt_mst = is_inst ? mmu->mmu_fch_expt_mst : mmu->ldst_expt_mst;

    if (ctx.expt_vld) {
        if (itf_fifo_full(expt_mst)) {
            return;
        }
        if (!is_inst && itf_fifo_full(va_rsp_mst)) {
            return;
        }

        itf_write(expt_mst, &ctx.expt);
        if (!is_inst) {
            itf_write(va_rsp_mst, &ctx.rsp);
        }
        ostq_free_head(ost);
        return;
    }

    if (itf_fifo_full(va_rsp_mst)) {
        return;
    }
    itf_write(va_rsp_mst, &ctx.rsp);
    ostq_free_head(ost);
}

void mmu_clock(mmu_t *mmu)
{
    mod_clock(&mmu->mod);
    ostq_clock(&mmu->i_ost);
    ostq_clock(&mmu->d_ost);
    mmu_recv_tlb_flush(mmu);
    mmu_drop_canceled_rsp(mmu);
    mmu_capture_req(mmu);
    mmu_proc_walk(mmu);
    mmu_proc_pa_rsp(mmu, true);
    mmu_proc_pa_rsp(mmu, false);
    mmu_send_ready_rsp(mmu, true);
    mmu_send_ready_rsp(mmu, false);
    mmu_accept_new_req(mmu);
    mmu_accept_new_req(mmu);
}

void mmu_free(mmu_t *mmu)
{
    mod_free(&mmu->mod);
    fifo_free(&mmu->va_i_req_fifo);
    fifo_free(&mmu->va_d_req_fifo);
    ostq_free(&mmu->i_ost);
    ostq_free(&mmu->d_ost);
}
