#ifndef MMU_H
#define MMU_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/hart_expt_if.h"
#include "spec/core/csr.h"

typedef struct mmu_conf {
} mmu_conf_t;

typedef struct mmu {
    itf_t *va_i_bti_req_slv;
    itf_t *va_i_bti_rsp_mst;
    itf_t *va_d_bti_req_slv;
    itf_t *va_d_bti_rsp_mst;
    itf_t *hart_expt_mst;

    itf_t *pa_i_bti_req_mst;
    itf_t *pa_i_bti_rsp_slv;
    itf_t *pa_d_bti_req_mst;
    itf_t *pa_d_bti_rsp_slv;

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
    int level;
    bool pte_req_pending;
    bool final_req_pending;
    bool final_rsp_pending;
} mmu_t;

extern void mmu_construct(mmu_t *mmu, const char *name, const mmu_conf_t *conf);
extern void mmu_reset(mmu_t *mmu);
extern void mmu_clock(mmu_t *mmu);
extern void mmu_free(mmu_t *mmu);


#endif
