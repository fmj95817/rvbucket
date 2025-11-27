#ifndef APB_REQ_IF_H
#define APB_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define APB_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(apb_req_if_t), \
        .pkt2str = &apb_req_if_to_str, \
        .reg_vcd = &apb_req_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_signal_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define APB_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(apb_req_if_t), \
        .pkt2str = &apb_req_if_to_str, \
        .reg_vcd = &apb_req_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct apb_req_if {
    u32 paddr;
    bool pwrite;
    u32 pwdata;
    u8 pstrb; // 4-bit
} apb_req_if_t;

static inline void apb_req_if_to_str(const void *pkt, char *str)
{
    const apb_req_if_t *apb_req = (const apb_req_if_t *)pkt;
    sprintf(str, "%08x %01x %08x %01x\n", apb_req->paddr, apb_req->pwrite, apb_req->pwdata, apb_req->pstrb);
}

static inline void apb_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const apb_req_if_t *apb_req = (const apb_req_if_t *)pkt;
    dbg_vcd_add_sig("paddr", type, 32, &apb_req->paddr);
    dbg_vcd_add_sig("pwrite", type, 1, &apb_req->pwrite);
    dbg_vcd_add_sig("pwdata", type, 32, &apb_req->pwdata);
    dbg_vcd_add_sig("pstrb", type, 4, &apb_req->pstrb);
}

#endif
