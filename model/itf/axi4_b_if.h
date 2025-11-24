#ifndef AXI4_B_IF_H
#define AXI4_B_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define AXI4_B_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &axi4_b_if_to_str, \
        .reg_vcd = &axi4_b_if_reg_vcd, \
        .pkt_size = sizeof(axi4_b_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct axi4_b_if {
    u8 id;
    u8 resp; // 2-bit
} axi4_b_if_t;

static inline void axi4_b_if_to_str(const void *pkt, char *str)
{
    const axi4_b_if_t *axi4_b = (const axi4_b_if_t *)pkt;
    sprintf(str, "%02x %01x\n", axi4_b->id, axi4_b->resp);
}

static inline void axi4_b_if_reg_vcd(const void *pkt)
{
    const axi4_b_if_t *axi4_b = (const axi4_b_if_t *)pkt;
    dbg_vcd_add_sig("id", DBG_SIG_TYPE_REG, 8, &axi4_b->id);
    dbg_vcd_add_sig("resp", DBG_SIG_TYPE_REG, 2, &axi4_b->resp);
}

#endif
