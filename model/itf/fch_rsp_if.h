#ifndef FCH_RSP_IF_H
#define FCH_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define FCH_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(fch_rsp_if_t), \
        .pkt2str = &fch_rsp_if_to_str, \
        .reg_vcd = &fch_rsp_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_signal_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define FCH_RSP_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(fch_rsp_if_t), \
        .pkt2str = &fch_rsp_if_to_str, \
        .reg_vcd = &fch_rsp_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct fch_rsp_if {
    u32 ir;
    bool ok;
} fch_rsp_if_t;

static inline void fch_rsp_if_to_str(const void *pkt, char *str)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", fch_rsp->ir, fch_rsp->ok);
}

static inline void fch_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    dbg_vcd_add_sig("ir", type, 32, &fch_rsp->ir);
    dbg_vcd_add_sig("ok", type, 1, &fch_rsp->ok);
}

#endif
