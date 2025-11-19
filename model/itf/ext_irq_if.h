#ifndef EXT_IRQ_IF_H
#define EXT_IRQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define EXT_IRQ_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &ext_irq_if_to_str, &ext_irq_if_reg_vcd_sig, sizeof(ext_irq_if_t), depth)

typedef struct ext_irq_if {
    bool set;
    bool clear;
} ext_irq_if_t;

static inline void ext_irq_if_to_str(const void *pkt, char *str)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    sprintf(str, "%01x %01x\n", ext_irq->set, ext_irq->clear);
}

static inline void ext_irq_if_reg_vcd_sig(const void *pkt)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    dbg_vcd_add_sig("set", DBG_SIG_TYPE_REG, 1, &ext_irq->set);
    dbg_vcd_add_sig("clear", DBG_SIG_TYPE_REG, 1, &ext_irq->clear);
}

#endif
