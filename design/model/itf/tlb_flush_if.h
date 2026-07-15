#ifndef TLB_FLUSH_IF_H
#define TLB_FLUSH_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define TLB_FLUSH_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    TLB_FLUSH_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define TLB_FLUSH_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    TLB_FLUSH_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define TLB_FLUSH_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(tlb_flush_if_t), \
        .pkt2str = &tlb_flush_if_to_str, \
        .reg_vcd = &tlb_flush_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define TLB_FLUSH_IF_CONSTRUCT(module, itf, depth) \
    TLB_FLUSH_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define TLB_FLUSH_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    TLB_FLUSH_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define TLB_FLUSH_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(tlb_flush_if_t), \
        .pkt2str = &tlb_flush_if_to_str, \
        .reg_vcd = &tlb_flush_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct tlb_flush_if {
    u32 dummy;
} tlb_flush_if_t;

static inline void tlb_flush_if_to_str(const void *pkt, char *str)
{
    const tlb_flush_if_t *tlb_flush = (const tlb_flush_if_t *)pkt;
    sprintf(str, "%08x\n", tlb_flush->dummy);
}

static inline void tlb_flush_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const tlb_flush_if_t *tlb_flush = (const tlb_flush_if_t *)pkt;
    dbg_vcd_add_sig("dummy", type, 32, &tlb_flush->dummy);
}

#endif
