#ifndef FCH_REQ_IF_H
#define FCH_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define FCH_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(fch_req_if_t), \
        .pkt2str = &fch_req_if_to_str, \
        .reg_vcd = &fch_req_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define FCH_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(fch_req_if_t), \
        .pkt2str = &fch_req_if_to_str, \
        .reg_vcd = &fch_req_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct fch_req_if {
    u32 pc;
} fch_req_if_t;

static inline void fch_req_if_to_str(const void *pkt, char *str)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    sprintf(str, "%08x\n", fch_req->pc);
}

static inline void fch_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    dbg_vcd_add_sig("pc", type, 32, &fch_req->pc);
}

#endif
