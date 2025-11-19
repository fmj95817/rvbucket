#ifndef APB_REQ_H
#define APB_REQ_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

typedef struct apb_req_if {
    u32 paddr;
    bool pwrite;
    u32 pwdata;
    u8 pstrb; /* 4-bit */
} apb_req_if_t;

static inline void apb_req_if_to_str(const void *pkt, char *str)
{
    const apb_req_if_t *apb_req = (const apb_req_if_t *)pkt;
    sprintf(str, "%08x %01x %08x %01x\n", apb_req->paddr, apb_req->pwrite, apb_req->pwdata, apb_req->pstrb);
}

static inline void apb_req_if_reg_vcd_sig(const void *pkt)
{
    const apb_req_if_t *apb_req = (const apb_req_if_t *)pkt;
    dbg_vcd_add_sig("paddr", DBG_SIG_TYPE_REG, 32, &apb_req->paddr);
    dbg_vcd_add_sig("pwrite", DBG_SIG_TYPE_REG, 1, &apb_req->pwrite);
    dbg_vcd_add_sig("pwdata", DBG_SIG_TYPE_REG, 32, &apb_req->pwdata);
    dbg_vcd_add_sig("pstrb", DBG_SIG_TYPE_REG, 4, &apb_req->pstrb);
}

#endif
