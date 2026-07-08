#ifndef EXU_STATE_IF_H
#define EXU_STATE_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define EXU_STATE_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(exu_state_if_t), \
        .pkt2str = &exu_state_if_to_str, \
        .reg_vcd = &exu_state_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define EXU_STATE_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(exu_state_if_t), \
        .pkt2str = &exu_state_if_to_str, \
        .reg_vcd = &exu_state_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct exu_state_if {
    u8 priv; // 2-bit
    u32 pc;
    u32 irq_epc;
    bool irq_defer;
    bool wfi;
    u32 wfi_resume_pc;
} exu_state_if_t;

static inline void exu_state_if_to_str(const void *pkt, char *str)
{
    const exu_state_if_t *exu_state = (const exu_state_if_t *)pkt;
    sprintf(str, "%01x %08x %08x %01x %01x %08x\n", exu_state->priv, exu_state->pc, exu_state->irq_epc, exu_state->irq_defer, exu_state->wfi, exu_state->wfi_resume_pc);
}

static inline void exu_state_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const exu_state_if_t *exu_state = (const exu_state_if_t *)pkt;
    dbg_vcd_add_sig("priv", type, 2, &exu_state->priv);
    dbg_vcd_add_sig("pc", type, 32, &exu_state->pc);
    dbg_vcd_add_sig("irq_epc", type, 32, &exu_state->irq_epc);
    dbg_vcd_add_sig("irq_defer", type, 1, &exu_state->irq_defer);
    dbg_vcd_add_sig("wfi", type, 1, &exu_state->wfi);
    dbg_vcd_add_sig("wfi_resume_pc", type, 32, &exu_state->wfi_resume_pc);
}

#endif
