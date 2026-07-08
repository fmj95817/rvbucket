#ifndef CSR_LSU_STATE_IF_H
#define CSR_LSU_STATE_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define CSR_LSU_STATE_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(csr_lsu_state_if_t), \
        .pkt2str = &csr_lsu_state_if_to_str, \
        .reg_vcd = &csr_lsu_state_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CSR_LSU_STATE_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(csr_lsu_state_if_t), \
        .pkt2str = &csr_lsu_state_if_to_str, \
        .reg_vcd = &csr_lsu_state_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct csr_lsu_state_if {
    u32 satp;
} csr_lsu_state_if_t;

static inline void csr_lsu_state_if_to_str(const void *pkt, char *str)
{
    const csr_lsu_state_if_t *csr_lsu_state = (const csr_lsu_state_if_t *)pkt;
    sprintf(str, "%08x\n", csr_lsu_state->satp);
}

static inline void csr_lsu_state_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const csr_lsu_state_if_t *csr_lsu_state = (const csr_lsu_state_if_t *)pkt;
    dbg_vcd_add_sig("satp", type, 32, &csr_lsu_state->satp);
}

#endif
