#ifndef PLIC_H
#define PLIC_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"
#include "itf/ext_irq_if.h"
#include "spec/core.h"

typedef struct plic_conf {
} plic_conf_t;

typedef struct plic {
    itf_t *cfg_apb_req_slv;
    itf_t *cfg_apb_rsp_mst;
    itf_t *div_ext_irq_ins[PLIC_MAX_IRQ_NUM];
    itf_t *conv_ext_irq_out;
} plic_t;

extern void plic_construct(plic_t *plic, const char *name, const plic_conf_t *conf);
extern void plic_reset(plic_t *plic);
extern void plic_clock(plic_t *plic);
extern void plic_free(plic_t *plic);

#endif