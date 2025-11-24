#ifndef AXI4_W_IF_H
#define AXI4_W_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define AXI4_W_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &axi4_w_if_to_str, \
        .reg_vcd = &axi4_w_if_reg_vcd, \
        .pkt_size = sizeof(axi4_w_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct axi4_w_if {
    u32 data;
    u8 strb; // 4-bit
    bool last;
} axi4_w_if_t;

static inline void axi4_w_if_to_str(const void *pkt, char *str)
{
    const axi4_w_if_t *axi4_w = (const axi4_w_if_t *)pkt;
    sprintf(str, "%08x %01x %01x\n", axi4_w->data, axi4_w->strb, axi4_w->last);
}

static inline void axi4_w_if_reg_vcd(const void *pkt)
{
    const axi4_w_if_t *axi4_w = (const axi4_w_if_t *)pkt;
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &axi4_w->data);
    dbg_vcd_add_sig("strb", DBG_SIG_TYPE_REG, 4, &axi4_w->strb);
    dbg_vcd_add_sig("last", DBG_SIG_TYPE_REG, 1, &axi4_w->last);
}

#endif
