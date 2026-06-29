#ifndef TIMER_H
#define TIMER_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/ext_irq_if.h"

typedef struct timer {
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
} peri_timer_t;

extern void timer_construct(peri_timer_t *t, const char *name, u32 base, u32 size);
extern void timer_reset(peri_timer_t *t);
extern void timer_clock(peri_timer_t *t);
extern void timer_free(peri_timer_t *t);

#endif
