#ifndef BPU_PRED_RSP_IF_H
#define BPU_PRED_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define BPU_PRED_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    BPU_PRED_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define BPU_PRED_RSP_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    BPU_PRED_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define BPU_PRED_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(bpu_pred_rsp_if_t), \
        .pkt2str = &bpu_pred_rsp_if_to_str, \
        .reg_vcd = &bpu_pred_rsp_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define BPU_PRED_RSP_IF_CONSTRUCT(module, itf, depth) \
    BPU_PRED_RSP_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define BPU_PRED_RSP_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    BPU_PRED_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define BPU_PRED_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(bpu_pred_rsp_if_t), \
        .pkt2str = &bpu_pred_rsp_if_to_str, \
        .reg_vcd = &bpu_pred_rsp_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct bpu_pred_rsp_if {
    bool vld;
    bool is_ctrl;
    bool pred_taken;
    u32 pred_pc;
    bool cond_bht_hit;
    bool jalr_ras_hit;
    bool jalr_btb_hit;
    bool jalr_btb_miss;
} bpu_pred_rsp_if_t;

static inline void bpu_pred_rsp_if_to_str(const void *pkt, char *str)
{
    const bpu_pred_rsp_if_t *bpu_pred_rsp = (const bpu_pred_rsp_if_t *)pkt;
    sprintf(str, "%01x %01x %01x %08x %01x %01x %01x %01x\n", bpu_pred_rsp->vld, bpu_pred_rsp->is_ctrl, bpu_pred_rsp->pred_taken, bpu_pred_rsp->pred_pc, bpu_pred_rsp->cond_bht_hit, bpu_pred_rsp->jalr_ras_hit, bpu_pred_rsp->jalr_btb_hit, bpu_pred_rsp->jalr_btb_miss);
}

static inline void bpu_pred_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const bpu_pred_rsp_if_t *bpu_pred_rsp = (const bpu_pred_rsp_if_t *)pkt;
    dbg_vcd_add_sig("vld", type, 1, &bpu_pred_rsp->vld);
    dbg_vcd_add_sig("is_ctrl", type, 1, &bpu_pred_rsp->is_ctrl);
    dbg_vcd_add_sig("pred_taken", type, 1, &bpu_pred_rsp->pred_taken);
    dbg_vcd_add_sig("pred_pc", type, 32, &bpu_pred_rsp->pred_pc);
    dbg_vcd_add_sig("cond_bht_hit", type, 1, &bpu_pred_rsp->cond_bht_hit);
    dbg_vcd_add_sig("jalr_ras_hit", type, 1, &bpu_pred_rsp->jalr_ras_hit);
    dbg_vcd_add_sig("jalr_btb_hit", type, 1, &bpu_pred_rsp->jalr_btb_hit);
    dbg_vcd_add_sig("jalr_btb_miss", type, 1, &bpu_pred_rsp->jalr_btb_miss);
}

#endif
