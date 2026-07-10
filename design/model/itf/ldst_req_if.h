#ifndef LDST_REQ_IF_H
#define LDST_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define LDST_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    LDST_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define LDST_REQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    LDST_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define LDST_REQ_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(ldst_req_if_t), \
        .pkt2str = &ldst_req_if_to_str, \
        .reg_vcd = &ldst_req_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define LDST_REQ_IF_CONSTRUCT(module, itf, depth) \
    LDST_REQ_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define LDST_REQ_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    LDST_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define LDST_REQ_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(ldst_req_if_t), \
        .pkt2str = &ldst_req_if_to_str, \
        .reg_vcd = &ldst_req_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum ldst_req_size {
    LDST_REQ_SIZE_B1 = 0,
    LDST_REQ_SIZE_B2 = 1,
    LDST_REQ_SIZE_B4 = 2
} ldst_req_size_t;

typedef struct ldst_req_if {
    u32 addr;
    bool st;
    ldst_req_size_t size;
    u32 data;
    u8 strobe; // 4-bit
} ldst_req_if_t;

static inline void ldst_req_if_to_str(const void *pkt, char *str)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    sprintf(str, "%08x %01x %01x %08x %01x\n", ldst_req->addr, ldst_req->st, (u32)ldst_req->size, ldst_req->data, ldst_req->strobe);
}

static inline void ldst_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    dbg_vcd_add_sig("addr", type, 32, &ldst_req->addr);
    dbg_vcd_add_sig("st", type, 1, &ldst_req->st);
    dbg_vcd_add_sig("size", type, 2, &ldst_req->size);
    dbg_vcd_add_sig("data", type, 32, &ldst_req->data);
    dbg_vcd_add_sig("strobe", type, 4, &ldst_req->strobe);
}

#endif
