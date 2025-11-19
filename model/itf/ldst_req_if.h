#ifndef LDST_REQ_IF_H
#define LDST_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define LDST_REQ_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &ldst_req_if_to_str, &ldst_req_if_reg_vcd_sig, sizeof(ldst_req_if_t), depth)

typedef struct ldst_req_if {
    u32 addr;
    bool st;
    u32 data;
    u8 strobe;
} ldst_req_if_t;

static inline void ldst_req_if_to_str(const void *pkt, char *str)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    sprintf(str, "%08x %01x %08x %02x\n", ldst_req->addr, ldst_req->st, ldst_req->data, ldst_req->strobe);
}

static inline void ldst_req_if_reg_vcd_sig(const void *pkt)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    dbg_vcd_add_sig("addr", DBG_SIG_TYPE_REG, 32, &ldst_req->addr);
    dbg_vcd_add_sig("st", DBG_SIG_TYPE_REG, 1, &ldst_req->st);
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &ldst_req->data);
    dbg_vcd_add_sig("strobe", DBG_SIG_TYPE_REG, 8, &ldst_req->strobe);
}

#endif
