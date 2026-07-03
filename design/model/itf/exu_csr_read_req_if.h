#ifndef EXU_CSR_READ_REQ_IF_H
#define EXU_CSR_READ_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define EXU_CSR_READ_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(exu_csr_read_req_if_t), \
        .pkt2str = &exu_csr_read_req_if_to_str, \
        .reg_vcd = &exu_csr_read_req_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define EXU_CSR_READ_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(exu_csr_read_req_if_t), \
        .pkt2str = &exu_csr_read_req_if_to_str, \
        .reg_vcd = &exu_csr_read_req_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct exu_csr_read_req_if {
    u16 addr; // 12-bit
    u8 priv; // 2-bit
} exu_csr_read_req_if_t;

static inline void exu_csr_read_req_if_to_str(const void *pkt, char *str)
{
    const exu_csr_read_req_if_t *exu_csr_read_req = (const exu_csr_read_req_if_t *)pkt;
    sprintf(str, "%03x %01x\n", exu_csr_read_req->addr, exu_csr_read_req->priv);
}

static inline void exu_csr_read_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const exu_csr_read_req_if_t *exu_csr_read_req = (const exu_csr_read_req_if_t *)pkt;
    dbg_vcd_add_sig("addr", type, 12, &exu_csr_read_req->addr);
    dbg_vcd_add_sig("priv", type, 2, &exu_csr_read_req->priv);
}

#endif
