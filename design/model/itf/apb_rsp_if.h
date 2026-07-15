#ifndef APB_RSP_IF_H
#define APB_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define APB_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    APB_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define APB_RSP_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    APB_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define APB_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(apb_rsp_if_t), \
        .pkt2str = &apb_rsp_if_to_str, \
        .reg_vcd = &apb_rsp_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define APB_RSP_IF_CONSTRUCT(module, itf, depth) \
    APB_RSP_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define APB_RSP_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    APB_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define APB_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(apb_rsp_if_t), \
        .pkt2str = &apb_rsp_if_to_str, \
        .reg_vcd = &apb_rsp_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct apb_rsp_if {
    u32 prdata;
    bool pslverr;
} apb_rsp_if_t;

static inline void apb_rsp_if_to_str(const void *pkt, char *str)
{
    const apb_rsp_if_t *apb_rsp = (const apb_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", apb_rsp->prdata, apb_rsp->pslverr);
}

static inline void apb_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const apb_rsp_if_t *apb_rsp = (const apb_rsp_if_t *)pkt;
    dbg_vcd_add_sig("prdata", type, 32, &apb_rsp->prdata);
    dbg_vcd_add_sig("pslverr", type, 1, &apb_rsp->pslverr);
}

#endif
