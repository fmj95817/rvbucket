#ifndef LDST_REQ_IF_H
#define LDST_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define LDST_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(ldst_req_if_t), \
        .pkt2str = &ldst_req_if_to_str, \
        .reg_vcd = &ldst_req_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define LDST_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(ldst_req_if_t), \
        .pkt2str = &ldst_req_if_to_str, \
        .reg_vcd = &ldst_req_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

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

static inline void ldst_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    dbg_vcd_add_sig("addr", type, 32, &ldst_req->addr);
    dbg_vcd_add_sig("st", type, 1, &ldst_req->st);
    dbg_vcd_add_sig("data", type, 32, &ldst_req->data);
    dbg_vcd_add_sig("strobe", type, 8, &ldst_req->strobe);
}

#endif
