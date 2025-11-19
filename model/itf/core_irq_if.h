#ifndef CORE_IRQ_IF_H
#define CORE_IRQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define CORE_IRQ_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &core_irq_if_to_str, &core_irq_if_reg_vcd_sig, sizeof(core_irq_if_t), depth)

typedef struct core_irq_if {
    bool mtimer;
    bool stimer;
    bool msw;
    bool ssw;
} core_irq_if_t;

static inline void core_irq_if_to_str(const void *pkt, char *str)
{
    const core_irq_if_t *core_irq = (const core_irq_if_t *)pkt;
    sprintf(str, "%01x %01x %01x %01x\n", core_irq->mtimer, core_irq->stimer, core_irq->msw, core_irq->ssw);
}

static inline void core_irq_if_reg_vcd_sig(const void *pkt)
{
    const core_irq_if_t *core_irq = (const core_irq_if_t *)pkt;
    dbg_vcd_add_sig("mtimer", DBG_SIG_TYPE_REG, 1, &core_irq->mtimer);
    dbg_vcd_add_sig("stimer", DBG_SIG_TYPE_REG, 1, &core_irq->stimer);
    dbg_vcd_add_sig("msw", DBG_SIG_TYPE_REG, 1, &core_irq->msw);
    dbg_vcd_add_sig("ssw", DBG_SIG_TYPE_REG, 1, &core_irq->ssw);
}

#endif
