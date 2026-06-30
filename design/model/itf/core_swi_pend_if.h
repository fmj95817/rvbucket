#ifndef CORE_SWI_PEND_IF_H
#define CORE_SWI_PEND_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define CORE_SWI_PEND_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(core_swi_pend_if_t), \
        .pkt2str = &core_swi_pend_if_to_str, \
        .reg_vcd = &core_swi_pend_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CORE_SWI_PEND_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(core_swi_pend_if_t), \
        .pkt2str = &core_swi_pend_if_to_str, \
        .reg_vcd = &core_swi_pend_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct core_swi_pend_if {
    bool msip;
} core_swi_pend_if_t;

static inline void core_swi_pend_if_to_str(const void *pkt, char *str)
{
    const core_swi_pend_if_t *core_swi_pend = (const core_swi_pend_if_t *)pkt;
    sprintf(str, "%01x\n", core_swi_pend->msip);
}

static inline void core_swi_pend_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const core_swi_pend_if_t *core_swi_pend = (const core_swi_pend_if_t *)pkt;
    dbg_vcd_add_sig("msip", type, 1, &core_swi_pend->msip);
}

#endif
