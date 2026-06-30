#ifndef ACLINT_H
#define ACLINT_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/core_timer_if.h"
#include "itf/core_m_irq_if.h"
#include "itf/core_s_irq_if.h"
#include "itf/core_swi_pend_if.h"
#include "spec/core/core.h"

typedef struct aclint_conf {
    u32 mtimer_base;
    u32 mtimer_size;
    u32 mtimecmp_base;
    u32 mtimecmp_size;
    u32 mswi_base;
    u32 mswi_size;
    u32 sswi_base;
    u32 sswi_size;
    u32 mtimer_tick_cycles;
} aclint_conf_t;

typedef union aclint_reg64 {
    u64 raw;
    struct {
        u32 lo;
        u32 hi;
    } reg;
} aclint_reg64_t;

typedef struct aclint {
    const u64 *cycle;
    itf_t *cfg_apb_req_slv;
    itf_t *cfg_apb_rsp_mst;
    itf_t *core_timer_out;
    itf_t *core_m_irq_outs[HART_NUM];
    itf_t *core_s_irq_msts[HART_NUM];
    itf_t *core_swi_pend_ins[HART_NUM];

    core_timer_if_t *core_timer_o;
    core_m_irq_if_t *core_m_irq_o[HART_NUM];
    const core_swi_pend_if_t *core_swi_pend_i[HART_NUM];

    aclint_conf_t conf;
    u32 mtime_cycle_cnt;

    aclint_reg64_t mtime;
    aclint_reg64_t mtimecmp[HART_NUM];
    bool mtime_exceed_old[HART_NUM];
} aclint_t;

extern void aclint_construct(aclint_t *aclint, const char *name, const aclint_conf_t *conf);
extern void aclint_reset(aclint_t *aclint);
extern void aclint_clock(aclint_t *aclint);
extern void aclint_free(aclint_t *aclint);

#endif