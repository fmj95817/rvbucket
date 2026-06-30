#ifndef BTI_RSP_IF_H
#define BTI_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define BTI_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(bti_rsp_if_t), \
        .pkt2str = &bti_rsp_if_to_str, \
        .reg_vcd = &bti_rsp_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define BTI_RSP_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(bti_rsp_if_t), \
        .pkt2str = &bti_rsp_if_to_str, \
        .reg_vcd = &bti_rsp_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

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

static inline void bti_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const bti_rsp_if_t *bti_rsp = (const bti_rsp_if_t *)pkt;
    dbg_vcd_add_sig("trans_id", type, 16, &bti_rsp->trans_id);
    dbg_vcd_add_sig("data", type, 32, &bti_rsp->data);
    dbg_vcd_add_sig("ok", type, 1, &bti_rsp->ok);
}

#endif
