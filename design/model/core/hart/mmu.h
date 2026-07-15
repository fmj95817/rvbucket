#ifndef MMU_H
#define MMU_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/bti_if.h"
#include "itf/hart_expt_if.h"
#include "itf/tlb_flush_if.h"
#include "itf/exu_state_if.h"
#include "itf/csr_mmu_state_if.h"
#include "spec/core/csr.h"
#include "spec/core/hart.h"

typedef struct mmu_conf {
    u32 i_stg_fifo_depth;
    u32 d_stg_fifo_depth;
    u32 ost_depth;
} mmu_conf_t;

typedef struct mmu_tlb_entry {
    bool vld;
    u32 tag;
    u32 satp_ppn;
    u32 satp_asid;
    rv32g_priv_t priv;
    bool sum;
    bool mxr;
    int level;
    u32 pte;
    u32 pa_base;
} mmu_tlb_entry_t;

typedef struct mmu_ost_ctx {
    bool is_inst;
    bti_req_if_t req;
    bool pa_issued;
    bool ready;
    bool expt_vld;
    hart_expt_if_t expt;
    bti_rsp_if_t rsp;
} mmu_ost_ctx_t;

typedef struct mmu {
    mod_t mod;
    itf_t *va_i_bti_req_slv;
    itf_t *va_i_bti_rsp_mst;
    itf_t *va_d_bti_req_slv;
    itf_t *va_d_bti_rsp_mst;
    itf_t *mmu_fch_expt_mst;
    itf_t *ldst_expt_mst;

    itf_t *pa_i_bti_req_mst;
    itf_t *pa_i_bti_rsp_slv;
    itf_t *pa_d_bti_req_mst;
    itf_t *pa_d_bti_rsp_slv;
    itf_t *ptw_bti_req_mst;
    itf_t *ptw_bti_rsp_slv;
    itf_t *tlb_flush_slv;

    itf_t *exu_state_in;
    itf_t *csr_mmu_state_in;
    const exu_state_if_t *exu_state_i;
    const csr_mmu_state_if_t *csr_state_i;

    fifo_t va_i_req_fifo;
    fifo_t va_d_req_fifo;
    ostq_t i_ost;
    ostq_t d_ost;
    bool walk_slot_vld;
    bool walk_is_inst;
    u32 walk_slot;

    bool busy;
    bool is_inst;
    bti_req_if_t req;
    rv32g_priv_t req_priv;
    bool req_sum;
    bool req_mxr;
    u32 fault_pc;
    u32 root_base;
    u32 va;
    u32 pte;
    u32 leaf_pa_base;
    int level;
    bool pte_req_pending;
    bool final_req_pending;
    bool final_rsp_pending;
    bool drop_ptw_rsp;
    bool drop_i_rsp;
    bool bare_i_rsp_pending;
    u32 itlb_replace_idx;
    u32 dtlb_replace_idx;
    mmu_tlb_entry_t itlb[MMU_ITLB_SIZE];
    mmu_tlb_entry_t dtlb[MMU_DTLB_SIZE];

    u64 *perf_itlb_hit;
    u64 *perf_itlb_miss;
    u64 *perf_dtlb_hit;
    u64 *perf_dtlb_miss;
    u64 *perf_i_stg_full;
    u64 *perf_d_stg_full;
    u64 *perf_i_ost_full;
    u64 *perf_d_ost_full;
} mmu_t;

extern void mmu_construct(mmu_t *mmu, const char *parent, const char *name, const mmu_conf_t *conf);
extern void mmu_reset(mmu_t *mmu);
extern void mmu_clock(mmu_t *mmu);
extern void mmu_free(mmu_t *mmu);


#endif
