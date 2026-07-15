#ifndef CSR_TRAP_STATE_IF_H
#define CSR_TRAP_STATE_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define CSR_TRAP_STATE_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \
    CSR_TRAP_STATE_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)

#define CSR_TRAP_STATE_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \
    CSR_TRAP_STATE_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)

#define CSR_TRAP_STATE_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(csr_trap_state_if_t), \
        .pkt2str = &csr_trap_state_if_to_str, \
        .reg_vcd = &csr_trap_state_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src, \
        .sim_prot = sim_prot_ \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CSR_TRAP_STATE_IF_CONSTRUCT(module, itf, depth) \
    CSR_TRAP_STATE_IF_CONSTRUCT_INNER(module, itf, depth, false)

#define CSR_TRAP_STATE_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \
    CSR_TRAP_STATE_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)

#define CSR_TRAP_STATE_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(csr_trap_state_if_t), \
        .pkt2str = &csr_trap_state_if_to_str, \
        .reg_vcd = &csr_trap_state_if_reg_vcd, \
        .force_disable_trace = false, \
        .sim_prot = sim_prot_, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct csr_trap_state_if {
    u32 mstatus;
    u32 mip;
    u32 mie;
    u32 mtvec;
    u32 mepc;
    u32 medeleg;
    u32 mideleg;
    u32 sstatus;
    u32 sip;
    u32 sie;
    u32 stvec;
    u32 sepc;
} csr_trap_state_if_t;

static inline void csr_trap_state_if_to_str(const void *pkt, char *str)
{
    const csr_trap_state_if_t *csr_trap_state = (const csr_trap_state_if_t *)pkt;
    sprintf(str, "%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", csr_trap_state->mstatus, csr_trap_state->mip, csr_trap_state->mie, csr_trap_state->mtvec, csr_trap_state->mepc, csr_trap_state->medeleg, csr_trap_state->mideleg, csr_trap_state->sstatus, csr_trap_state->sip, csr_trap_state->sie, csr_trap_state->stvec, csr_trap_state->sepc);
}

static inline void csr_trap_state_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const csr_trap_state_if_t *csr_trap_state = (const csr_trap_state_if_t *)pkt;
    dbg_vcd_add_sig("mstatus", type, 32, &csr_trap_state->mstatus);
    dbg_vcd_add_sig("mip", type, 32, &csr_trap_state->mip);
    dbg_vcd_add_sig("mie", type, 32, &csr_trap_state->mie);
    dbg_vcd_add_sig("mtvec", type, 32, &csr_trap_state->mtvec);
    dbg_vcd_add_sig("mepc", type, 32, &csr_trap_state->mepc);
    dbg_vcd_add_sig("medeleg", type, 32, &csr_trap_state->medeleg);
    dbg_vcd_add_sig("mideleg", type, 32, &csr_trap_state->mideleg);
    dbg_vcd_add_sig("sstatus", type, 32, &csr_trap_state->sstatus);
    dbg_vcd_add_sig("sip", type, 32, &csr_trap_state->sip);
    dbg_vcd_add_sig("sie", type, 32, &csr_trap_state->sie);
    dbg_vcd_add_sig("stvec", type, 32, &csr_trap_state->stvec);
    dbg_vcd_add_sig("sepc", type, 32, &csr_trap_state->sepc);
}

#endif
