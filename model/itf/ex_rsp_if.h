#ifndef EX_RSP_IF_H
#define EX_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define EX_RSP_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &ex_rsp_if_to_str, \
        .reg_vcd = &ex_rsp_if_reg_vcd, \
        .pkt_size = sizeof(ex_rsp_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct ex_rsp_if {
    u32 pc;
    bool taken;
    bool pred_true;
    u32 target_pc;
} ex_rsp_if_t;

static inline void ex_rsp_if_to_str(const void *pkt, char *str)
{
    const ex_rsp_if_t *ex_rsp = (const ex_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x %01x %08x\n", ex_rsp->pc, ex_rsp->taken, ex_rsp->pred_true, ex_rsp->target_pc);
}

static inline void ex_rsp_if_reg_vcd(const void *pkt)
{
    const ex_rsp_if_t *ex_rsp = (const ex_rsp_if_t *)pkt;
    dbg_vcd_add_sig("pc", DBG_SIG_TYPE_REG, 32, &ex_rsp->pc);
    dbg_vcd_add_sig("taken", DBG_SIG_TYPE_REG, 1, &ex_rsp->taken);
    dbg_vcd_add_sig("pred_true", DBG_SIG_TYPE_REG, 1, &ex_rsp->pred_true);
    dbg_vcd_add_sig("target_pc", DBG_SIG_TYPE_REG, 32, &ex_rsp->target_pc);
}

#endif
