#ifndef AXI4_AW_IF_H
#define AXI4_AW_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define AXI4_AW_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    AXI4_AW_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define AXI4_AW_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    AXI4_AW_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define AXI4_AW_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(axi4_aw_if_t), \
        .pkt2str = &axi4_aw_if_to_str, \
        .reg_vcd = &axi4_aw_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define AXI4_AW_IF_CONSTRUCT(module, itf, depth) \
    AXI4_AW_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define AXI4_AW_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    AXI4_AW_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define AXI4_AW_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(axi4_aw_if_t), \
        .pkt2str = &axi4_aw_if_to_str, \
        .reg_vcd = &axi4_aw_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum axi4_aw_burst {
    AXI4_AW_BURST_FIXED = 0,
    AXI4_AW_BURST_INCR = 1,
    AXI4_AW_BURST_WRAP = 2
} axi4_aw_burst_t;

typedef enum axi4_aw_size {
    AXI4_AW_SIZE_B1 = 0,
    AXI4_AW_SIZE_B2 = 1,
    AXI4_AW_SIZE_B4 = 2,
    AXI4_AW_SIZE_B8 = 3,
    AXI4_AW_SIZE_B16 = 4,
    AXI4_AW_SIZE_B32 = 5,
    AXI4_AW_SIZE_B64 = 6,
    AXI4_AW_SIZE_B128 = 7
} axi4_aw_size_t;

typedef struct axi4_aw_if {
    u8 id;
    u32 addr;
    u8 len;
    axi4_aw_size_t size;
    axi4_aw_burst_t burst;
    bool lock;
    u8 cache; // 4-bit
    u8 prot; // 3-bit
    u8 qos; // 4-bit
    u32 user;
} axi4_aw_if_t;

static inline void axi4_aw_if_to_str(const void *pkt, char *str)
{
    const axi4_aw_if_t *axi4_aw = (const axi4_aw_if_t *)pkt;
    sprintf(str, "%02x %08x %02x %01x %01x %01x %01x %01x %01x %08x\n", axi4_aw->id, axi4_aw->addr, axi4_aw->len, (u32)axi4_aw->size, (u32)axi4_aw->burst, axi4_aw->lock, axi4_aw->cache, axi4_aw->prot, axi4_aw->qos, axi4_aw->user);
}

static inline void axi4_aw_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const axi4_aw_if_t *axi4_aw = (const axi4_aw_if_t *)pkt;
    dbg_vcd_add_sig("id", type, 8, &axi4_aw->id);
    dbg_vcd_add_sig("addr", type, 32, &axi4_aw->addr);
    dbg_vcd_add_sig("len", type, 8, &axi4_aw->len);
    dbg_vcd_add_sig("size", type, 3, &axi4_aw->size);
    dbg_vcd_add_sig("burst", type, 2, &axi4_aw->burst);
    dbg_vcd_add_sig("lock", type, 1, &axi4_aw->lock);
    dbg_vcd_add_sig("cache", type, 4, &axi4_aw->cache);
    dbg_vcd_add_sig("prot", type, 3, &axi4_aw->prot);
    dbg_vcd_add_sig("qos", type, 4, &axi4_aw->qos);
    dbg_vcd_add_sig("user", type, 32, &axi4_aw->user);
}

#endif
