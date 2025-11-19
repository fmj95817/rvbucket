#ifndef APB_RSP_H
#define APB_RSP_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

typedef struct apb_rsp_if {
    u32 prdata;
    bool pslverr;
} apb_rsp_if_t;

static inline void apb_rsp_if_to_str(const void *pkt, char *str)
{
    const apb_rsp_if_t *apb_rsp = (const apb_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", apb_rsp->prdata, apb_rsp->pslverr);
}

static inline void apb_rsp_if_reg_vcd_sig(const void *pkt)
{
    const apb_rsp_if_t *apb_rsp = (const apb_rsp_if_t *)pkt;
    dbg_vcd_add_sig("prdata", DBG_SIG_TYPE_REG, 32, &apb_rsp->prdata);
    dbg_vcd_add_sig("pslverr", DBG_SIG_TYPE_REG, 1, &apb_rsp->pslverr);
}

#endif
