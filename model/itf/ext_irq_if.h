#ifndef EXT_IRQ_H
#define EXT_IRQ_H

#include <stdio.h>
#include "base/types.h"

typedef struct ext_irq_if {
    u32 dummy;
} ext_irq_if_t;

static inline void ext_irq_if_to_str(const void *pkt, char *str)
{
    const ext_irq_if_t *ext_irq = (const ext_irq_if_t *)pkt;
    sprintf(str, "%08x\n", ext_irq->dummy);
}

#endif
