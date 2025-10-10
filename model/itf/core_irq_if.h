#ifndef CORE_IRQ_H
#define CORE_IRQ_H

#include <stdio.h>
#include "base/types.h"

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

#endif
