#ifndef AXI4_R_IF_H
#define AXI4_R_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define AXI4_R_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &axi4_r_if_to_str, &axi4_r_if_reg_vcd_sig, sizeof(axi4_r_if_t), depth)

typedef struct axi4_r_if {
    u8 id;
    u32 data;
    u8 resp; // 2-bit
    bool last;
} axi4_r_if_t;

static inline void axi4_r_if_to_str(const void *pkt, char *str)
{
    const axi4_r_if_t *axi4_r = (const axi4_r_if_t *)pkt;
    sprintf(str, "%02x %08x %01x %01x\n", axi4_r->id, axi4_r->data, axi4_r->resp, axi4_r->last);
}

static inline void axi4_r_if_reg_vcd_sig(const void *pkt)
{
    const axi4_r_if_t *axi4_r = (const axi4_r_if_t *)pkt;
    dbg_vcd_add_sig("id", DBG_SIG_TYPE_REG, 8, &axi4_r->id);
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &axi4_r->data);
    dbg_vcd_add_sig("resp", DBG_SIG_TYPE_REG, 2, &axi4_r->resp);
    dbg_vcd_add_sig("last", DBG_SIG_TYPE_REG, 1, &axi4_r->last);
}

#endif
