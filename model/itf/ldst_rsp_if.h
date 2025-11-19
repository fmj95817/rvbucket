#ifndef LDST_RSP_IF_H
#define LDST_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define LDST_RSP_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &ldst_rsp_if_to_str, &ldst_rsp_if_reg_vcd_sig, sizeof(ldst_rsp_if_t), depth)

typedef struct ldst_rsp_if {
    u32 data;
    bool ok;
} ldst_rsp_if_t;

static inline void ldst_rsp_if_to_str(const void *pkt, char *str)
{
    const ldst_rsp_if_t *ldst_rsp = (const ldst_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", ldst_rsp->data, ldst_rsp->ok);
}

static inline void ldst_rsp_if_reg_vcd_sig(const void *pkt)
{
    const ldst_rsp_if_t *ldst_rsp = (const ldst_rsp_if_t *)pkt;
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &ldst_rsp->data);
    dbg_vcd_add_sig("ok", DBG_SIG_TYPE_REG, 1, &ldst_rsp->ok);
}

#endif
