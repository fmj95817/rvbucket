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

void mmu_construct(mmu_t *mmu, const char *name, const mmu_conf_t *conf)
{
    (void)conf;
    DBG_VCD_MODULE_SCOPE(name);
    mmu->perf_itlb_hit = dbg_pcm_reg_perf_cnt("mmu_itlb_hit");
    mmu->perf_itlb_miss = dbg_pcm_reg_perf_cnt("mmu_itlb_miss");
    mmu->perf_dtlb_hit = dbg_pcm_reg_perf_cnt("mmu_dtlb_hit");
    mmu->perf_dtlb_miss = dbg_pcm_reg_perf_cnt("mmu_dtlb_miss");
}

void mmu_reset(mmu_t *mmu)
{
    mmu->busy = false;
    mmu->pte_req_pending = false;
    mmu->final_req_pending = false;
    mmu->final_rsp_pending = false;
    mmu->drop_ptw_rsp = false;
    mmu->drop_i_rsp = false;
    mmu->bare_i_rsp_pending = false;
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
}

static rv32g_priv_t mmu_effective_priv(const mmu_t *mmu, mmu_access_t access)
{
    rv32g_priv_t priv = *mmu->priv;
    if (access != MMU_ACCESS_INST && priv == RV32G_PRIV_MACHINE &&
        mmu->mstatus->reg.mprv) {
        return (rv32g_priv_t)mmu->mstatus->reg.mpp;
    }
    return priv;
}

static bool mmu_translation_enabled(const mmu_t *mmu, mmu_access_t access)
{
    rv32g_priv_t priv = mmu_effective_priv(mmu, access);
    return priv != RV32G_PRIV_MACHINE &&
        mmu->satp->reg.mode == MMU_SATP_MODE_SV32;
}

static void mmu_send_expt(mmu_t *mmu, hart_expt_cause_t cause, u32 pc, u32 tval)
{
    hart_expt_if_t pkt;
    pkt.type = HART_EXPT_TYPE_EXCEPTION;
    pkt.cause = cause;
    pkt.priv = mmu->req_priv;
    pkt.pc = pc;
    pkt.tval = tval;
    itf_write(mmu->hart_expt_mst, &pkt);
}

static void mmu_send_fault_rsp(mmu_t *mmu)
{
    bti_rsp_if_t rsp;
    rsp.trans_id = mmu->req.trans_id;
    rsp.data = 0;
    rsp.ok = false;
    itf_write(mmu->is_inst ? mmu->va_i_bti_rsp_mst : mmu->va_d_bti_rsp_mst, &rsp);
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

    mmu_send_expt(mmu, cause, mmu->fault_pc, mmu->va);
    mmu_send_fault_rsp(mmu);
    mmu->busy = false;
    mmu->pte_req_pending = false;
    mmu->final_req_pending = false;
    mmu->final_rsp_pending = false;
}

static bool mmu_forward_rsp(mmu_t *mmu, itf_t *pa_rsp_slv, itf_t *va_rsp_mst)
{
    if (itf_fifo_empty(pa_rsp_slv) || itf_fifo_full(va_rsp_mst)) {
        return false;
    }

    bti_rsp_if_t rsp;
    itf_read(pa_rsp_slv, &rsp);
    itf_write(va_rsp_mst, &rsp);
    return true;
}

static void mmu_send_pa_req(mmu_t *mmu, itf_t *pa_req_mst, const bti_req_if_t *req)
{
    DBG_CHECK(!itf_fifo_full(pa_req_mst));
    itf_write(pa_req_mst, req);
}

static void mmu_start_req(mmu_t *mmu, bool is_inst, const bti_req_if_t *req)
{
    mmu_access_t access = is_inst ? MMU_ACCESS_INST :
        (req->cmd == BTI_REQ_CMD_WRITE ? MMU_ACCESS_STORE : MMU_ACCESS_LOAD);

    mmu->busy = true;
    mmu->is_inst = is_inst;
    mmu->req = *req;
    mmu->req_priv = mmu_effective_priv(mmu, access);
    mmu->req_sum = mmu->mstatus->reg.sum;
    mmu->req_mxr = mmu->mstatus->reg.mxr;
    mmu->fault_pc = is_inst ? req->addr : *mmu->exu_pc;
    mmu->va = req->addr;
    mmu->root_base = mmu->satp->reg.ppn << MMU_PGSHIFT;
    mmu->level = 1;
    mmu->pte_req_pending = false;
    mmu->final_req_pending = false;
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

static bool mmu_tlb_lookup(mmu_t *mmu, bool is_inst, const bti_req_if_t *req)
{
    u32 num;
    mmu_tlb_entry_t *entries = mmu_tlb_entries(mmu, is_inst, &num);
    mmu_access_t access = is_inst ? MMU_ACCESS_INST :
        (req->cmd == BTI_REQ_CMD_WRITE ? MMU_ACCESS_STORE : MMU_ACCESS_LOAD);
    rv32g_priv_t req_priv = mmu_effective_priv(mmu, access);
    bool req_sum = mmu->mstatus->reg.sum;
    bool req_mxr = mmu->mstatus->reg.mxr;

    for (u32 i = 0; i < num; i++) {
        mmu_tlb_entry_t *entry = entries + i;
        if (!entry->vld) {
            continue;
        }
        if (entry->satp_ppn != mmu->satp->reg.ppn ||
            entry->satp_asid != mmu->satp->reg.asid ||
            entry->priv != req_priv ||
            entry->sum != req_sum ||
            entry->mxr != req_mxr ||
            entry->tag != mmu_tlb_tag(req->addr, entry->level)) {
            continue;
        }

        mmu->busy = true;
        mmu->is_inst = is_inst;
        mmu->req = *req;
        mmu->req_priv = req_priv;
        mmu->req_sum = req_sum;
        mmu->req_mxr = req_mxr;
        mmu->fault_pc = is_inst ? req->addr : *mmu->exu_pc;
        mmu->va = req->addr;
        mmu->pte = entry->pte;
        if (!mmu_pte_permits(mmu, entry->pte) || !mmu_pte_ad_ok(mmu, entry->pte)) {
            mmu_raise_fault(mmu);
            return true;
        }

        mmu->req.addr = entry->pa_base | (req->addr & mmu_tlb_page_mask(entry->level));
        mmu->final_req_pending = true;
        mmu->final_rsp_pending = false;
        mmu->pte_req_pending = false;
        if (is_inst) {
            (*mmu->perf_itlb_hit)++;
        } else {
            (*mmu->perf_dtlb_hit)++;
        }
        return true;
    }

    if (is_inst) {
        (*mmu->perf_itlb_miss)++;
    } else {
        (*mmu->perf_dtlb_miss)++;
    }
    return false;
}

static void mmu_tlb_fill(mmu_t *mmu)
{
    u32 num;
    mmu_tlb_entry_t *entries = mmu_tlb_entries(mmu, mmu->is_inst, &num);
    u32 *replace_idx = mmu->is_inst ? &mmu->itlb_replace_idx : &mmu->dtlb_replace_idx;
    mmu_tlb_entry_t *entry = entries + *replace_idx;

    entry->vld = true;
    entry->tag = mmu_tlb_tag(mmu->va, mmu->level);
    entry->satp_ppn = mmu->satp->reg.ppn;
    entry->satp_asid = mmu->satp->reg.asid;
    entry->priv = mmu->req_priv;
    entry->sum = mmu->req_sum;
    entry->mxr = mmu->req_mxr;
    entry->level = mmu->level;
    entry->pte = mmu->pte;
    entry->pa_base = mmu->leaf_pa_base & ~mmu_tlb_page_mask(mmu->level);

    *replace_idx = (*replace_idx + 1u) % num;
}

static void mmu_accept_new_req(mmu_t *mmu)
{
    if (mmu->busy) {
        return;
    }

    if (mmu->drop_ptw_rsp || mmu->drop_i_rsp) {
        return;
    }

    if (!itf_fifo_empty(mmu->va_d_bti_req_slv)) {
        if (!mmu_translation_enabled(mmu, MMU_ACCESS_LOAD) &&
            itf_fifo_full(mmu->pa_d_bti_req_mst)) {
            return;
        }

        bti_req_if_t req;
        itf_read(mmu->va_d_bti_req_slv, &req);

        mmu_access_t access = req.cmd == BTI_REQ_CMD_WRITE ?
            MMU_ACCESS_STORE : MMU_ACCESS_LOAD;
        if (!mmu_translation_enabled(mmu, access)) {
            if (itf_fifo_full(mmu->pa_d_bti_req_mst)) {
                return;
            }
            mmu_send_pa_req(mmu, mmu->pa_d_bti_req_mst, &req);
            return;
        }

        if (!mmu_tlb_lookup(mmu, false, &req)) {
            mmu_start_req(mmu, false, &req);
        }
        return;
    }

    if (!itf_fifo_empty(mmu->va_i_bti_req_slv)) {
        if (!mmu_translation_enabled(mmu, MMU_ACCESS_INST) &&
            itf_fifo_full(mmu->pa_i_bti_req_mst)) {
            return;
        }

        bti_req_if_t req;
        itf_read(mmu->va_i_bti_req_slv, &req);

        if (!mmu_translation_enabled(mmu, MMU_ACCESS_INST)) {
            if (itf_fifo_full(mmu->pa_i_bti_req_mst)) {
                return;
            }
            mmu_send_pa_req(mmu, mmu->pa_i_bti_req_mst, &req);
            mmu->bare_i_rsp_pending = true;
            return;
        }

        if (!mmu_tlb_lookup(mmu, true, &req)) {
            mmu_start_req(mmu, true, &req);
        }
    }
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

    bti_req_if_t req;
    req.trans_id = MMU_PTE_TRANS_ID;
    req.cmd = BTI_REQ_CMD_READ;
    req.addr = mmu_pte_addr(mmu);
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
    itf_t *va_rsp_mst = mmu->is_inst ? mmu->va_i_bti_rsp_mst : mmu->va_d_bti_rsp_mst;
    if (itf_fifo_empty(pa_rsp_slv) || itf_fifo_full(va_rsp_mst)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_read(pa_rsp_slv, &rsp);
    itf_write(va_rsp_mst, &rsp);
    mmu->final_rsp_pending = false;
    mmu->busy = false;
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

void mmu_clock(mmu_t *mmu)
{
    mmu_recv_tlb_flush(mmu);
    mmu_drop_canceled_rsp(mmu);

    if (!mmu->busy) {
        if (mmu_forward_rsp(mmu, mmu->pa_i_bti_rsp_slv, mmu->va_i_bti_rsp_mst)) {
            mmu->bare_i_rsp_pending = false;
        }
    }

    if (!mmu->busy) {
        (void)mmu_forward_rsp(mmu, mmu->pa_d_bti_rsp_slv, mmu->va_d_bti_rsp_mst);
    }

    mmu_proc_walk(mmu);
    mmu_accept_new_req(mmu);
}

void mmu_free(mmu_t *mmu)
{
    (void)mmu;
}
