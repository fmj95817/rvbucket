#ifndef FL_REQ_IF_H
#define FL_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define FL_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    FL_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define FL_REQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    FL_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define FL_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(fl_req_if_t), \
        .pkt2str = &fl_req_if_to_str, \
        .reg_vcd = &fl_req_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define FL_REQ_IF_CONSTRUCT(module, itf, depth) \
    FL_REQ_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define FL_REQ_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    FL_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define FL_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(fl_req_if_t), \
        .pkt2str = &fl_req_if_to_str, \
        .reg_vcd = &fl_req_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct fl_req_if {
    u32 dummy;
} fl_req_if_t;

static inline void fl_req_if_to_str(const void *pkt, char *str)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    sprintf(str, "%08x\n", fl_req->dummy);
}

static inline void fl_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    dbg_vcd_add_sig("dummy", type, 32, &fl_req->dummy);
}

#endif
