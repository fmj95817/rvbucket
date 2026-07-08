#ifndef AXI4_AR_IF_H
#define AXI4_AR_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define AXI4_AR_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(axi4_ar_if_t), \
        .pkt2str = &axi4_ar_if_to_str, \
        .reg_vcd = &axi4_ar_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define AXI4_AR_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(axi4_ar_if_t), \
        .pkt2str = &axi4_ar_if_to_str, \
        .reg_vcd = &axi4_ar_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum axi4_ar_burst {
    AXI4_AR_BURST_FIXED = 0,
    AXI4_AR_BURST_INCR = 1,
    AXI4_AR_BURST_WRAP = 2
} axi4_ar_burst_t;

typedef enum axi4_ar_size {
    AXI4_AR_SIZE_B1 = 0,
    AXI4_AR_SIZE_B2 = 1,
    AXI4_AR_SIZE_B4 = 2,
    AXI4_AR_SIZE_B8 = 3,
    AXI4_AR_SIZE_B16 = 4,
    AXI4_AR_SIZE_B32 = 5,
    AXI4_AR_SIZE_B64 = 6,
    AXI4_AR_SIZE_B128 = 7
} axi4_ar_size_t;

typedef struct axi4_ar_if {
    u8 id;
    u32 addr;
    u8 len;
    axi4_ar_size_t size;
    axi4_ar_burst_t burst;
    bool lock;
    u8 cache; // 4-bit
    u8 prot; // 3-bit
    u8 qos; // 4-bit
    u32 user;
} axi4_ar_if_t;

static inline void axi4_ar_if_to_str(const void *pkt, char *str)
{
    const axi4_ar_if_t *axi4_ar = (const axi4_ar_if_t *)pkt;
    sprintf(str, "%02x %08x %02x %01x %01x %01x %01x %01x %01x %08x\n", axi4_ar->id, axi4_ar->addr, axi4_ar->len, (u32)axi4_ar->size, (u32)axi4_ar->burst, axi4_ar->lock, axi4_ar->cache, axi4_ar->prot, axi4_ar->qos, axi4_ar->user);
}

static inline void axi4_ar_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const axi4_ar_if_t *axi4_ar = (const axi4_ar_if_t *)pkt;
    dbg_vcd_add_sig("id", type, 8, &axi4_ar->id);
    dbg_vcd_add_sig("addr", type, 32, &axi4_ar->addr);
    dbg_vcd_add_sig("len", type, 8, &axi4_ar->len);
    dbg_vcd_add_sig("size", type, 3, &axi4_ar->size);
    dbg_vcd_add_sig("burst", type, 2, &axi4_ar->burst);
    dbg_vcd_add_sig("lock", type, 1, &axi4_ar->lock);
    dbg_vcd_add_sig("cache", type, 4, &axi4_ar->cache);
    dbg_vcd_add_sig("prot", type, 3, &axi4_ar->prot);
    dbg_vcd_add_sig("qos", type, 4, &axi4_ar->qos);
    dbg_vcd_add_sig("user", type, 32, &axi4_ar->user);
}

#endif
