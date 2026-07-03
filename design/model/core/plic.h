#ifndef PLIC_H
#define PLIC_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/ext_irq_if.h"
#include "spec/core/core.h"

typedef struct plic_conf {
} plic_conf_t;

typedef struct plic {
    mod_t mod;
    itf_t *cfg_apb_req_slv;
    itf_t *cfg_apb_rsp_mst;
    itf_t *div_ext_irq_ins[PLIC_MAX_IRQ_NUM];
    itf_t *conv_ext_irq_out;

    ext_irq_if_t *ext_irq_o;
    u32 priority[PLIC_SOURCE_NUM];
    u32 pending[PLIC_BITFIELD_WORDS];
    u32 claimed[PLIC_BITFIELD_WORDS];
    u32 enable[PLIC_BITFIELD_WORDS];
    u32 threshold;
} plic_t;

extern void plic_construct(plic_t *plic, const char *parent, const char *name, const plic_conf_t *conf);
extern void plic_reset(plic_t *plic);
extern void plic_clock(plic_t *plic);
extern void plic_free(plic_t *plic);

#endif
