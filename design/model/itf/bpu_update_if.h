#ifndef BPU_UPDATE_IF_H
#define BPU_UPDATE_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define BPU_UPDATE_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    BPU_UPDATE_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define BPU_UPDATE_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    BPU_UPDATE_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define BPU_UPDATE_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(bpu_update_if_t), \
        .pkt2str = &bpu_update_if_to_str, \
        .reg_vcd = &bpu_update_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define BPU_UPDATE_IF_CONSTRUCT(module, itf, depth) \
    BPU_UPDATE_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define BPU_UPDATE_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    BPU_UPDATE_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define BPU_UPDATE_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(bpu_update_if_t), \
        .pkt2str = &bpu_update_if_to_str, \
        .reg_vcd = &bpu_update_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct bpu_update_if {
    bool vld;
    u32 pc;
    u32 ir;
    bool taken;
    u32 target_pc;
    bool pred_taken;
    u32 pred_pc;
    bool pred_true;
    bool is_boot_code;
    bool pred_cond_bht_hit;
    bool pred_jalr_ras_hit;
    bool pred_jalr_btb_hit;
    bool pred_jalr_btb_miss;
} bpu_update_if_t;

static inline void bpu_update_if_to_str(const void *pkt, char *str)
{
    const bpu_update_if_t *bpu_update = (const bpu_update_if_t *)pkt;
    sprintf(str, "%01x %08x %08x %01x %08x %01x %08x %01x %01x %01x %01x %01x %01x\n", bpu_update->vld, bpu_update->pc, bpu_update->ir, bpu_update->taken, bpu_update->target_pc, bpu_update->pred_taken, bpu_update->pred_pc, bpu_update->pred_true, bpu_update->is_boot_code, bpu_update->pred_cond_bht_hit, bpu_update->pred_jalr_ras_hit, bpu_update->pred_jalr_btb_hit, bpu_update->pred_jalr_btb_miss);
}

static inline void bpu_update_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const bpu_update_if_t *bpu_update = (const bpu_update_if_t *)pkt;
    dbg_vcd_add_sig("vld", type, 1, &bpu_update->vld);
    dbg_vcd_add_sig("pc", type, 32, &bpu_update->pc);
    dbg_vcd_add_sig("ir", type, 32, &bpu_update->ir);
    dbg_vcd_add_sig("taken", type, 1, &bpu_update->taken);
    dbg_vcd_add_sig("target_pc", type, 32, &bpu_update->target_pc);
    dbg_vcd_add_sig("pred_taken", type, 1, &bpu_update->pred_taken);
    dbg_vcd_add_sig("pred_pc", type, 32, &bpu_update->pred_pc);
    dbg_vcd_add_sig("pred_true", type, 1, &bpu_update->pred_true);
    dbg_vcd_add_sig("is_boot_code", type, 1, &bpu_update->is_boot_code);
    dbg_vcd_add_sig("pred_cond_bht_hit", type, 1, &bpu_update->pred_cond_bht_hit);
    dbg_vcd_add_sig("pred_jalr_ras_hit", type, 1, &bpu_update->pred_jalr_ras_hit);
    dbg_vcd_add_sig("pred_jalr_btb_hit", type, 1, &bpu_update->pred_jalr_btb_hit);
    dbg_vcd_add_sig("pred_jalr_btb_miss", type, 1, &bpu_update->pred_jalr_btb_miss);
}

#endif
