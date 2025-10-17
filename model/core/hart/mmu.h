#ifndef MMU_H
#define MMU_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"

typedef struct mmu_conf {
} mmu_conf_t;

typedef struct mmu {
    itf_t *va_i_bti_req_slv;
    itf_t *va_i_bti_rsp_mst;
    itf_t *va_d_bti_req_slv;
    itf_t *va_d_bti_rsp_mst;

    itf_t *pa_i_bti_req_mst;
    itf_t *pa_i_bti_rsp_slv;
    itf_t *pa_d_bti_req_mst;
    itf_t *pa_d_bti_rsp_slv;
} mmu_t;

extern void mmu_construct(mmu_t *mmu, const mmu_conf_t *conf);
extern void mmu_reset(mmu_t *mmu);
extern void mmu_clock(mmu_t *mmu);
extern void mmu_free(mmu_t *mmu);


#endif