#ifndef CSR_TRAP_WRITE_RSP_IF_H
#define CSR_TRAP_WRITE_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define CSR_TRAP_WRITE_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(csr_trap_write_rsp_if_t), \
        .pkt2str = &csr_trap_write_rsp_if_to_str, \
        .reg_vcd = &csr_trap_write_rsp_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CSR_TRAP_WRITE_RSP_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(csr_trap_write_rsp_if_t), \
        .pkt2str = &csr_trap_write_rsp_if_to_str, \
        .reg_vcd = &csr_trap_write_rsp_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct csr_trap_write_rsp_if {
    bool ok;
} csr_trap_write_rsp_if_t;

static inline void csr_trap_write_rsp_if_to_str(const void *pkt, char *str)
{
    const csr_trap_write_rsp_if_t *csr_trap_write_rsp = (const csr_trap_write_rsp_if_t *)pkt;
    sprintf(str, "%01x\n", csr_trap_write_rsp->ok);
}

static inline void csr_trap_write_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const csr_trap_write_rsp_if_t *csr_trap_write_rsp = (const csr_trap_write_rsp_if_t *)pkt;
    dbg_vcd_add_sig("ok", type, 1, &csr_trap_write_rsp->ok);
}

#endif
