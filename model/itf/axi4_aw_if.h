#ifndef AXI4_AW_IF_H
#define AXI4_AW_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define AXI4_AW_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &axi4_aw_if_to_str, \
        .reg_vcd = &axi4_aw_if_reg_vcd, \
        .pkt_size = sizeof(axi4_aw_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct axi4_aw_if {
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
} axi4_aw_if_t;

static inline void axi4_aw_if_to_str(const void *pkt, char *str)
{
    const axi4_aw_if_t *axi4_aw = (const axi4_aw_if_t *)pkt;
    sprintf(str, "%02x %08x %02x %01x %01x %01x %01x %01x %01x %08x\n", axi4_aw->id, axi4_aw->addr, axi4_aw->len, axi4_aw->size, axi4_aw->burst, axi4_aw->lock, axi4_aw->cache, axi4_aw->prot, axi4_aw->qos, axi4_aw->user);
}

static inline void axi4_aw_if_reg_vcd(const void *pkt)
{
    const axi4_aw_if_t *axi4_aw = (const axi4_aw_if_t *)pkt;
    dbg_vcd_add_sig("id", DBG_SIG_TYPE_REG, 8, &axi4_aw->id);
    dbg_vcd_add_sig("addr", DBG_SIG_TYPE_REG, 32, &axi4_aw->addr);
    dbg_vcd_add_sig("len", DBG_SIG_TYPE_REG, 8, &axi4_aw->len);
    dbg_vcd_add_sig("size", DBG_SIG_TYPE_REG, 3, &axi4_aw->size);
    dbg_vcd_add_sig("burst", DBG_SIG_TYPE_REG, 2, &axi4_aw->burst);
    dbg_vcd_add_sig("lock", DBG_SIG_TYPE_REG, 1, &axi4_aw->lock);
    dbg_vcd_add_sig("cache", DBG_SIG_TYPE_REG, 4, &axi4_aw->cache);
    dbg_vcd_add_sig("prot", DBG_SIG_TYPE_REG, 3, &axi4_aw->prot);
    dbg_vcd_add_sig("qos", DBG_SIG_TYPE_REG, 4, &axi4_aw->qos);
    dbg_vcd_add_sig("user", DBG_SIG_TYPE_REG, 32, &axi4_aw->user);
}

#endif
