#ifndef CORE_S_IRQ_IF_H
#define CORE_S_IRQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define CORE_S_IRQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    CORE_S_IRQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define CORE_S_IRQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    CORE_S_IRQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define CORE_S_IRQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(core_s_irq_if_t), \
        .pkt2str = &core_s_irq_if_to_str, \
        .reg_vcd = &core_s_irq_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CORE_S_IRQ_IF_CONSTRUCT(module, itf, depth) \
    CORE_S_IRQ_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define CORE_S_IRQ_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    CORE_S_IRQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define CORE_S_IRQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(core_s_irq_if_t), \
        .pkt2str = &core_s_irq_if_to_str, \
        .reg_vcd = &core_s_irq_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct core_s_irq_if {
    bool ssw;
} core_s_irq_if_t;

static inline void core_s_irq_if_to_str(const void *pkt, char *str)
{
    const core_s_irq_if_t *core_s_irq = (const core_s_irq_if_t *)pkt;
    sprintf(str, "%01x\n", core_s_irq->ssw);
}

static inline void core_s_irq_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const core_s_irq_if_t *core_s_irq = (const core_s_irq_if_t *)pkt;
    dbg_vcd_add_sig("ssw", type, 1, &core_s_irq->ssw);
}

#endif
