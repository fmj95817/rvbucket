#ifndef BTI_RSP_IF_H
#define BTI_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define BTI_RSP_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &bti_rsp_if_to_str, &bti_rsp_if_reg_vcd_sig, sizeof(bti_rsp_if_t), depth)

typedef struct bti_rsp_if {
    u16 trans_id;
    u32 data;
    bool ok;
} bti_rsp_if_t;

static inline void bti_rsp_if_to_str(const void *pkt, char *str)
{
    const bti_rsp_if_t *bti_rsp = (const bti_rsp_if_t *)pkt;
    sprintf(str, "%04x %08x %01x\n", bti_rsp->trans_id, bti_rsp->data, bti_rsp->ok);
}

static inline void bti_rsp_if_reg_vcd_sig(const void *pkt)
{
    const bti_rsp_if_t *bti_rsp = (const bti_rsp_if_t *)pkt;
    dbg_vcd_add_sig("trans_id", DBG_SIG_TYPE_REG, 16, &bti_rsp->trans_id);
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &bti_rsp->data);
    dbg_vcd_add_sig("ok", DBG_SIG_TYPE_REG, 1, &bti_rsp->ok);
}

#endif
