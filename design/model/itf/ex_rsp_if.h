#ifndef EX_RSP_IF_H
#define EX_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define EX_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(ex_rsp_if_t), \
        .pkt2str = &ex_rsp_if_to_str, \
        .reg_vcd = &ex_rsp_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define EX_RSP_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(ex_rsp_if_t), \
        .pkt2str = &ex_rsp_if_to_str, \
        .reg_vcd = &ex_rsp_if_reg_vcd, \
        .force_disable_dump = false, \
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

static inline void ex_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const ex_rsp_if_t *ex_rsp = (const ex_rsp_if_t *)pkt;
    dbg_vcd_add_sig("pc", type, 32, &ex_rsp->pc);
    dbg_vcd_add_sig("taken", type, 1, &ex_rsp->taken);
    dbg_vcd_add_sig("pred_true", type, 1, &ex_rsp->pred_true);
    dbg_vcd_add_sig("target_pc", type, 32, &ex_rsp->target_pc);
}

#endif
