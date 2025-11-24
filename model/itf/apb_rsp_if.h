#ifndef APB_RSP_IF_H
#define APB_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define APB_RSP_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &apb_rsp_if_to_str, \
        .reg_vcd = &apb_rsp_if_reg_vcd, \
        .pkt_size = sizeof(apb_rsp_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct apb_rsp_if {
    u32 prdata;
    bool pslverr;
} apb_rsp_if_t;

static inline void apb_rsp_if_to_str(const void *pkt, char *str)
{
    const apb_rsp_if_t *apb_rsp = (const apb_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", apb_rsp->prdata, apb_rsp->pslverr);
}

static inline void apb_rsp_if_reg_vcd(const void *pkt)
{
    const apb_rsp_if_t *apb_rsp = (const apb_rsp_if_t *)pkt;
    dbg_vcd_add_sig("prdata", DBG_SIG_TYPE_REG, 32, &apb_rsp->prdata);
    dbg_vcd_add_sig("pslverr", DBG_SIG_TYPE_REG, 1, &apb_rsp->pslverr);
}

#endif
