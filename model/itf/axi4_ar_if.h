#ifndef AXI4_AR_IF_H
#define AXI4_AR_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define AXI4_AR_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &axi4_ar_if_to_str, \
        .reg_vcd = &axi4_ar_if_reg_vcd, \
        .pkt_size = sizeof(axi4_ar_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct axi4_ar_if {
    u8 id;
    u32 addr;
    u8 len;
    u8 size; // 3-bit
    u8 burst; // 2-bit
    bool lock;
    u8 cache; // 4-bit
    u8 prot; // 3-bit
    u8 qos; // 4-bit
    u32 user;
} axi4_ar_if_t;

static inline void axi4_ar_if_to_str(const void *pkt, char *str)
{
    const axi4_ar_if_t *axi4_ar = (const axi4_ar_if_t *)pkt;
    sprintf(str, "%02x %08x %02x %01x %01x %01x %01x %01x %01x %08x\n", axi4_ar->id, axi4_ar->addr, axi4_ar->len, axi4_ar->size, axi4_ar->burst, axi4_ar->lock, axi4_ar->cache, axi4_ar->prot, axi4_ar->qos, axi4_ar->user);
}

static inline void axi4_ar_if_reg_vcd(const void *pkt)
{
    const axi4_ar_if_t *axi4_ar = (const axi4_ar_if_t *)pkt;
    dbg_vcd_add_sig("id", DBG_SIG_TYPE_REG, 8, &axi4_ar->id);
    dbg_vcd_add_sig("addr", DBG_SIG_TYPE_REG, 32, &axi4_ar->addr);
    dbg_vcd_add_sig("len", DBG_SIG_TYPE_REG, 8, &axi4_ar->len);
    dbg_vcd_add_sig("size", DBG_SIG_TYPE_REG, 3, &axi4_ar->size);
    dbg_vcd_add_sig("burst", DBG_SIG_TYPE_REG, 2, &axi4_ar->burst);
    dbg_vcd_add_sig("lock", DBG_SIG_TYPE_REG, 1, &axi4_ar->lock);
    dbg_vcd_add_sig("cache", DBG_SIG_TYPE_REG, 4, &axi4_ar->cache);
    dbg_vcd_add_sig("prot", DBG_SIG_TYPE_REG, 3, &axi4_ar->prot);
    dbg_vcd_add_sig("qos", DBG_SIG_TYPE_REG, 4, &axi4_ar->qos);
    dbg_vcd_add_sig("user", DBG_SIG_TYPE_REG, 32, &axi4_ar->user);
}

#endif
