#ifndef EXT_IRQ_IF_H
#define EXT_IRQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define EXT_IRQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(ext_irq_if_t), \
        .pkt2str = &ext_irq_if_to_str, \
        .reg_vcd = &ext_irq_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define EXT_IRQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(ext_irq_if_t), \
        .pkt2str = &ext_irq_if_to_str, \
        .reg_vcd = &ext_irq_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct ext_irq_if {
    bool irq;
} ext_irq_if_t;

static inline void ext_irq_if_to_str(const void *pkt, char *str)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    sprintf(str, "%01x\n", ext_irq->irq);
}

static inline void ext_irq_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    dbg_vcd_add_sig("irq", type, 1, &ext_irq->irq);
}

#endif
