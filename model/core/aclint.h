#ifndef ACLINT_H
#define ACLINT_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"
#include "itf/core_irq_if.h"

typedef struct aclint_conf {
    u32 mtimer_base;
    u32 mtimer_size;
    u32 mtimecmp_base;
    u32 mtimecmp_size;
    u32 mswi_base;
    u32 mswi_size;
    u32 sswi_base;
    u32 sswi_size;
} aclint_conf_t;

typedef struct aclint {
    const u64 *cycle;
    const u32 *csr_stimecmp;
    const u32 *csr_stimecmph;
    itf_t *cfg_apb_req_slv;
    itf_t *cfg_apb_rsp_mst;
    itf_t *core_irq_mst;
} aclint_t;

extern void aclint_construct(aclint_t *aclint, const char *name, const aclint_conf_t *conf);
extern void aclint_reset(aclint_t *aclint);
extern void aclint_clock(aclint_t *aclint);
extern void aclint_free(aclint_t *aclint);

#endif