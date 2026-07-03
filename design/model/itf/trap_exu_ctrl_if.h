#ifndef TRAP_EXU_CTRL_IF_H
#define TRAP_EXU_CTRL_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define TRAP_EXU_CTRL_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(trap_exu_ctrl_if_t), \
        .pkt2str = &trap_exu_ctrl_if_to_str, \
        .reg_vcd = &trap_exu_ctrl_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define TRAP_EXU_CTRL_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(trap_exu_ctrl_if_t), \
        .pkt2str = &trap_exu_ctrl_if_to_str, \
        .reg_vcd = &trap_exu_ctrl_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct trap_exu_ctrl_if {
    u8 priv; // 2-bit
    u32 irq_epc;
    bool wfi;
} trap_exu_ctrl_if_t;

static inline void trap_exu_ctrl_if_to_str(const void *pkt, char *str)
{
    const trap_exu_ctrl_if_t *trap_exu_ctrl = (const trap_exu_ctrl_if_t *)pkt;
    sprintf(str, "%01x %08x %01x\n", trap_exu_ctrl->priv, trap_exu_ctrl->irq_epc, trap_exu_ctrl->wfi);
}

static inline void trap_exu_ctrl_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const trap_exu_ctrl_if_t *trap_exu_ctrl = (const trap_exu_ctrl_if_t *)pkt;
    dbg_vcd_add_sig("priv", type, 2, &trap_exu_ctrl->priv);
    dbg_vcd_add_sig("irq_epc", type, 32, &trap_exu_ctrl->irq_epc);
    dbg_vcd_add_sig("wfi", type, 1, &trap_exu_ctrl->wfi);
}

#endif
