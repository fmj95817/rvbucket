#ifndef HART_EXPT_IF_H
#define HART_EXPT_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define HART_EXPT_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(hart_expt_if_t), \
        .pkt2str = &hart_expt_if_to_str, \
        .reg_vcd = &hart_expt_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define HART_EXPT_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(hart_expt_if_t), \
        .pkt2str = &hart_expt_if_to_str, \
        .reg_vcd = &hart_expt_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum hart_expt_type {
    HART_EXPT_TYPE_EXCEPTION = 0,
    HART_EXPT_TYPE_MRET = 1,
    HART_EXPT_TYPE_SRET = 2,
} hart_expt_type_t;
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
    hart_expt_type_t type;
    hart_expt_cause_t cause;
    u8 priv; // 2-bit
    u32 pc;
    u32 tval;
} hart_expt_if_t;

static inline void hart_expt_if_to_str(const void *pkt, char *str)
{
    const hart_expt_if_t *hart_expt = (const hart_expt_if_t *)pkt;
    sprintf(str, "%01x %02x %01x %08x %08x\n", (u32)hart_expt->type, (u32)hart_expt->cause, hart_expt->priv, hart_expt->pc, hart_expt->tval);
}

static inline void hart_expt_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const hart_expt_if_t *hart_expt = (const hart_expt_if_t *)pkt;
    dbg_vcd_add_sig("type", type, 2, &hart_expt->type);
    dbg_vcd_add_sig("cause", type, 5, &hart_expt->cause);
    dbg_vcd_add_sig("priv", type, 2, &hart_expt->priv);
    dbg_vcd_add_sig("pc", type, 32, &hart_expt->pc);
    dbg_vcd_add_sig("tval", type, 32, &hart_expt->tval);
}

#endif
