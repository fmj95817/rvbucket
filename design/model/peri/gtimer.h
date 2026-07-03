#ifndef TIMER_H
#define TIMER_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/ext_irq_if.h"

typedef struct gtimer {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *irq_out;

    u32 base_addr;
    u32 size;

    ext_irq_if_t *irq_o;
    u32 ctrl;       /* bit 0 = enable */
    u32 count;      /* current counter */
    u32 reload;     /* reload / preset */
    bool irq_pend;  /* interrupt pending */
} gtimer_t;

extern void gtimer_construct(gtimer_t *t, const char *parent, const char *name, u32 base, u32 size);
extern void gtimer_reset(gtimer_t *t);
extern void gtimer_clock(gtimer_t *t);
extern void gtimer_free(gtimer_t *t);

#endif
