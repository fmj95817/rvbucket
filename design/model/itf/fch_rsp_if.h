#ifndef FCH_RSP_IF_H
#define FCH_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define FCH_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    FCH_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define FCH_RSP_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    FCH_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define FCH_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(fch_rsp_if_t), \
        .pkt2str = &fch_rsp_if_to_str, \
        .reg_vcd = &fch_rsp_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define FCH_RSP_IF_CONSTRUCT(module, itf, depth) \
    FCH_RSP_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define FCH_RSP_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    FCH_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define FCH_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(fch_rsp_if_t), \
        .pkt2str = &fch_rsp_if_to_str, \
        .reg_vcd = &fch_rsp_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct fch_rsp_if {
    u32 ir;
    bool ok;
    bool expt;
    u32 cause;
    u8 priv; // 2-bit
    u32 tval;
} fch_rsp_if_t;

static inline void fch_rsp_if_to_str(const void *pkt, char *str)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x %01x %08x %01x %08x\n", fch_rsp->ir, fch_rsp->ok, fch_rsp->expt, fch_rsp->cause, fch_rsp->priv, fch_rsp->tval);
}

static inline void fch_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    dbg_vcd_add_sig("ir", type, 32, &fch_rsp->ir);
    dbg_vcd_add_sig("ok", type, 1, &fch_rsp->ok);
    dbg_vcd_add_sig("expt", type, 1, &fch_rsp->expt);
    dbg_vcd_add_sig("cause", type, 32, &fch_rsp->cause);
    dbg_vcd_add_sig("priv", type, 2, &fch_rsp->priv);
    dbg_vcd_add_sig("tval", type, 32, &fch_rsp->tval);
}

#endif
