#ifndef CSR_EXU_READ_RSP_IF_H
#define CSR_EXU_READ_RSP_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define CSR_EXU_READ_RSP_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    CSR_EXU_READ_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define CSR_EXU_READ_RSP_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    CSR_EXU_READ_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define CSR_EXU_READ_RSP_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(csr_exu_read_rsp_if_t), \
        .pkt2str = &csr_exu_read_rsp_if_to_str, \
        .reg_vcd = &csr_exu_read_rsp_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CSR_EXU_READ_RSP_IF_CONSTRUCT(module, itf, depth) \
    CSR_EXU_READ_RSP_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define CSR_EXU_READ_RSP_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    CSR_EXU_READ_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define CSR_EXU_READ_RSP_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(csr_exu_read_rsp_if_t), \
        .pkt2str = &csr_exu_read_rsp_if_to_str, \
        .reg_vcd = &csr_exu_read_rsp_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct csr_exu_read_rsp_if {
    u32 val;
    bool ok;
} csr_exu_read_rsp_if_t;

static inline void csr_exu_read_rsp_if_to_str(const void *pkt, char *str)
{
    const csr_exu_read_rsp_if_t *csr_exu_read_rsp = (const csr_exu_read_rsp_if_t *)pkt;
    sprintf(str, "%08x %01x\n", csr_exu_read_rsp->val, csr_exu_read_rsp->ok);
}

static inline void csr_exu_read_rsp_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const csr_exu_read_rsp_if_t *csr_exu_read_rsp = (const csr_exu_read_rsp_if_t *)pkt;
    dbg_vcd_add_sig("val", type, 32, &csr_exu_read_rsp->val);
    dbg_vcd_add_sig("ok", type, 1, &csr_exu_read_rsp->ok);
}

#endif
