#ifndef FCH_REQ_IF_H
#define FCH_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define FCH_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    FCH_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define FCH_REQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    FCH_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define FCH_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(fch_req_if_t), \
        .pkt2str = &fch_req_if_to_str, \
        .reg_vcd = &fch_req_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define FCH_REQ_IF_CONSTRUCT(module, itf, depth) \
    FCH_REQ_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define FCH_REQ_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    FCH_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define FCH_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(fch_req_if_t), \
        .pkt2str = &fch_req_if_to_str, \
        .reg_vcd = &fch_req_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct fch_req_if {
    u32 pc;
} fch_req_if_t;

static inline void fch_req_if_to_str(const void *pkt, char *str)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    sprintf(str, "%08x\n", fch_req->pc);
}

static inline void fch_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    dbg_vcd_add_sig("pc", type, 32, &fch_req->pc);
}

#endif
