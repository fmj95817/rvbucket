#ifndef TRAP_H
#define TRAP_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/hart_expt_if.h"
#include "itf/core_irq_if.h"
#include "itf/ext_irq_if.h"
#include "itf/trap_send_if.h"

typedef struct trap {
    itf_t *hart_expt_slv;
    itf_t *core_irq_slv;
    itf_t *ext_irq_slv;
    itf_t *trap_send_mst;
} trap_t;

extern void trap_construct(trap_t *trap);
extern void trap_reset(trap_t *trap);
extern void trap_clock(trap_t *trap);
extern void trap_free(trap_t *trap);

#endif