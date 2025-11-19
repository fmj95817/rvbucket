#ifndef HART_EXPT_H
#define HART_EXPT_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

typedef enum hart_expt_cause {
    HART_EXPT_CAUSE_INST_ADDR_MISALIGNED = 0,
    HART_EXPT_CAUSE_INST_ACCESS_FAULT = 1,
    HART_EXPT_CAUSE_ILLEGAL_INST = 2,
    HART_EXPT_CAUSE_BREAKPOINT = 3,
    HART_EXPT_CAUSE_LOAD_ADDR_MISALIGNED = 4,
    HART_EXPT_CAUSE_LOAD_ACCESS_FAULT = 5,
    HART_EXPT_CAUSE_STORE_AMO_ADDR_MISALIGNED = 6,
    HART_EXPT_CAUSE_STORE_AMO_ACCESS_FAULT = 7,
    HART_EXPT_CAUSE_ECALL = 8,
    HART_EXPT_CAUSE_INST_PAGE_FAULT = 12,
    HART_EXPT_CAUSE_LOAD_PAGE_FAULT = 13,
    HART_EXPT_CAUSE_STORE_AMO_PAGE_FAULT = 15,
    HART_EXPT_CAUSE_DOUBLE_TRAP = 16,
    HART_EXPT_CAUSE_SW_CHECK = 18,
    HART_EXPT_CAUSE_HW_ERROR = 19,
} hart_expt_cause_t;

typedef struct hart_expt_if {
    hart_expt_cause_t cause;
} hart_expt_if_t;

static inline void hart_expt_if_to_str(const void *pkt, char *str)
{
    const hart_expt_if_t *hart_expt = (const hart_expt_if_t *)pkt;
    sprintf(str, "%02x\n", (u32)hart_expt->cause);
}

static inline void hart_expt_if_reg_vcd_sig(const void *pkt)
{
    const hart_expt_if_t *hart_expt = (const hart_expt_if_t *)pkt;
    dbg_vcd_add_sig("cause", DBG_SIG_TYPE_REG, 5, &hart_expt->cause);
}

#endif
