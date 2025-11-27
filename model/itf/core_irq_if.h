#ifndef CORE_IRQ_IF_H
#define CORE_IRQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define CORE_IRQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(core_irq_if_t), \
        .pkt2str = &core_irq_if_to_str, \
        .reg_vcd = &core_irq_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_signal_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CORE_IRQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(core_irq_if_t), \
        .pkt2str = &core_irq_if_to_str, \
        .reg_vcd = &core_irq_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct core_irq_if {
    bool mtimer;
    bool msw;
    bool ssw;
} core_irq_if_t;

static inline void core_irq_if_to_str(const void *pkt, char *str)
{
    const core_irq_if_t *core_irq = (const core_irq_if_t *)pkt;
    sprintf(str, "%01x %01x %01x\n", core_irq->mtimer, core_irq->msw, core_irq->ssw);
}

static inline void core_irq_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const core_irq_if_t *core_irq = (const core_irq_if_t *)pkt;
    dbg_vcd_add_sig("mtimer", type, 1, &core_irq->mtimer);
    dbg_vcd_add_sig("msw", type, 1, &core_irq->msw);
    dbg_vcd_add_sig("ssw", type, 1, &core_irq->ssw);
}

#endif
