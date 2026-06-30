#ifndef MMU_H
#define MMU_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_if.h"
#include "itf/hart_expt_if.h"
#include "itf/tlb_flush_if.h"
#include "spec/core/csr.h"
#include "spec/core/hart.h"

typedef struct mmu_conf {
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

typedef struct mmu {
    itf_t *va_i_bti_req_slv;
    itf_t *va_i_bti_rsp_mst;
    itf_t *va_d_bti_req_slv;
    itf_t *va_d_bti_rsp_mst;
    itf_t *i_hart_expt_mst;
    itf_t *hart_expt_mst;

    itf_t *pa_i_bti_req_mst;
    itf_t *pa_i_bti_rsp_slv;
    itf_t *pa_d_bti_req_mst;
    itf_t *pa_d_bti_rsp_slv;
    itf_t *ptw_bti_req_mst;
    itf_t *ptw_bti_rsp_slv;
    itf_t *tlb_flush_slv;

    const rv32g_priv_t *priv;
    const rv32g_csr_satp_t *satp;
    const rv32g_csr_mstatus_t *mstatus;
    const u32 *ifu_pc;
    const u32 *exu_pc;

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
} mmu_t;

extern void mmu_construct(mmu_t *mmu, const char *name, const mmu_conf_t *conf);
extern void mmu_reset(mmu_t *mmu);
extern void mmu_clock(mmu_t *mmu);
extern void mmu_free(mmu_t *mmu);


#endif
