#ifndef TRAP_CSR_WRITE_REQ_IF_H
#define TRAP_CSR_WRITE_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define TRAP_CSR_WRITE_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(trap_csr_write_req_if_t), \
        .pkt2str = &trap_csr_write_req_if_to_str, \
        .reg_vcd = &trap_csr_write_req_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define TRAP_CSR_WRITE_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(trap_csr_write_req_if_t), \
        .pkt2str = &trap_csr_write_req_if_to_str, \
        .reg_vcd = &trap_csr_write_req_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct trap_csr_write_req_if {
    u16 addr; // 12-bit
    u32 val;
} trap_csr_write_req_if_t;

static inline void trap_csr_write_req_if_to_str(const void *pkt, char *str)
{
    const trap_csr_write_req_if_t *trap_csr_write_req = (const trap_csr_write_req_if_t *)pkt;
    sprintf(str, "%03x %08x\n", trap_csr_write_req->addr, trap_csr_write_req->val);
}

static inline void trap_csr_write_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const trap_csr_write_req_if_t *trap_csr_write_req = (const trap_csr_write_req_if_t *)pkt;
    dbg_vcd_add_sig("addr", type, 12, &trap_csr_write_req->addr);
    dbg_vcd_add_sig("val", type, 32, &trap_csr_write_req->val);
}

#endif
