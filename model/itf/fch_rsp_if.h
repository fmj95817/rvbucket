#ifndef FCH_RSP_IF_H
#define FCH_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define FCH_RSP_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &fch_rsp_if_to_str, &fch_rsp_if_reg_vcd_sig, sizeof(fch_rsp_if_t), depth)

typedef struct fch_rsp_if {
    u32 ir;
    bool ok;
} fch_rsp_if_t;

static inline void fch_rsp_if_to_str(const void *pkt, char *str)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", fch_rsp->ir, fch_rsp->ok);
}

static inline void fch_rsp_if_reg_vcd_sig(const void *pkt)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    dbg_vcd_add_sig("ir", DBG_SIG_TYPE_REG, 32, &fch_rsp->ir);
    dbg_vcd_add_sig("ok", DBG_SIG_TYPE_REG, 1, &fch_rsp->ok);
}

#endif
