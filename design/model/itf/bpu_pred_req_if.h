#ifndef BPU_PRED_REQ_IF_H
#define BPU_PRED_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define BPU_PRED_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    BPU_PRED_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define BPU_PRED_REQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    BPU_PRED_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define BPU_PRED_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(bpu_pred_req_if_t), \
        .pkt2str = &bpu_pred_req_if_to_str, \
        .reg_vcd = &bpu_pred_req_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define BPU_PRED_REQ_IF_CONSTRUCT(module, itf, depth) \
    BPU_PRED_REQ_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define BPU_PRED_REQ_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    BPU_PRED_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define BPU_PRED_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(bpu_pred_req_if_t), \
        .pkt2str = &bpu_pred_req_if_to_str, \
        .reg_vcd = &bpu_pred_req_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct bpu_pred_req_if {
    bool vld;
    u32 pc;
    u32 ir;
} bpu_pred_req_if_t;

static inline void bpu_pred_req_if_to_str(const void *pkt, char *str)
{
    const bpu_pred_req_if_t *bpu_pred_req = (const bpu_pred_req_if_t *)pkt;
    sprintf(str, "%01x %08x %08x\n", bpu_pred_req->vld, bpu_pred_req->pc, bpu_pred_req->ir);
}

static inline void bpu_pred_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const bpu_pred_req_if_t *bpu_pred_req = (const bpu_pred_req_if_t *)pkt;
    dbg_vcd_add_sig("vld", type, 1, &bpu_pred_req->vld);
    dbg_vcd_add_sig("pc", type, 32, &bpu_pred_req->pc);
    dbg_vcd_add_sig("ir", type, 32, &bpu_pred_req->ir);
}

#endif
