#ifndef EXT_IRQ_IF_H
#define EXT_IRQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define EXT_IRQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &ext_irq_if_to_str, \
        .reg_vcd = &ext_irq_if_reg_vcd, \
        .pkt_size = sizeof(ext_irq_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct ext_irq_if {
    bool set;
    bool clear;
} ext_irq_if_t;

static inline void ext_irq_if_to_str(const void *pkt, char *str)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    sprintf(str, "%01x %01x\n", ext_irq->set, ext_irq->clear);
}

static inline void ext_irq_if_reg_vcd(const void *pkt)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    dbg_vcd_add_sig("set", DBG_SIG_TYPE_REG, 1, &ext_irq->set);
    dbg_vcd_add_sig("clear", DBG_SIG_TYPE_REG, 1, &ext_irq->clear);
}

#endif
