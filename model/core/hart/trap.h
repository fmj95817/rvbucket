#ifndef TRAP_H
#define TRAP_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/hart_expt_if.h"
#include "itf/core_irq_if.h"
#include "itf/ext_irq_if.h"
#include "itf/trap_send_if.h"
#include "csr.h"

typedef struct trap {
    itf_t *hart_expt_slv;
    itf_t *core_irq_slv;
    itf_t *ext_irq_slv;
    itf_t *trap_send_mst;

    rv32g_priv_t *priv;
    const rv32g_csr_mstatus_t *mstatus;
    const rv32g_csr_mtvec_t *mtvec;
    const rv32g_csr_mepc_t *mepc;
    const rv32g_csr_mcause_t *mcause;
    const rv32g_csr_mie_t *mie;
    const rv32g_csr_mip_t *mip;
    const rv32g_csr_mtval_t *mtval;
    const rv32g_csr_mscratch_t *mscratch;
    const rv32g_csr_medeleg_t *medeleg;
    const rv32g_csr_mideleg_t *mideleg;
    const rv32g_csr_sstatus_t *sstatus;
    const rv32g_csr_stvec_t *stvec;
    const rv32g_csr_sepc_t *sepc;
    const rv32g_csr_scause_t *scause;
    const rv32g_csr_sie_t *sie;
    const rv32g_csr_sip_t *sip;
    const rv32g_csr_stval_t *stval;
    const rv32g_csr_sscratch_t *sscratch;
} trap_t;

extern void trap_construct(trap_t *trap);
extern void trap_reset(trap_t *trap);
extern void trap_clock(trap_t *trap);
extern void trap_free(trap_t *trap);

#endif