#ifndef TRAP_H
#define TRAP_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/hart_expt_if.h"
#include "itf/core_m_irq_if.h"
#include "itf/core_s_irq_if.h"
#include "itf/ext_irq_if.h"
#include "itf/trap_send_if.h"
#include "spec/csr.h"

typedef struct trap {
    itf_t *hart_expt_slv;
    itf_t *trap_send_mst;
    itf_t *fl_req_mst;
    itf_t *core_m_irq_in;
    itf_t *ext_irq_in;

    rv32g_priv_t *priv;

    rv32g_csr_mstatus_t *mstatus;
    rv32g_csr_mcause_t *mcause;
    rv32g_csr_mip_t *mip;
    rv32g_csr_mie_t *mie;
    rv32g_csr_mtvec_t *mtvec;
    rv32g_csr_mepc_t *mepc;
    rv32g_csr_mtval_t *mtval;
    rv32g_csr_medeleg_t *medeleg;
    rv32g_csr_mideleg_t *mideleg;

    rv32g_csr_sstatus_t *sstatus;
    rv32g_csr_scause_t *scause;
    rv32g_csr_sip_t *sip;
    rv32g_csr_sie_t *sie;
    rv32g_csr_stvec_t *stvec;
    rv32g_csr_sepc_t *sepc;
    rv32g_csr_stval_t *stval;

    const core_m_irq_if_t *core_m_irq_i;
    const ext_irq_if_t *ext_irq_i;

    u32 *ifu_pc;
    bool *exu_wfi;
    u32 *exu_wfi_resume_pc;
} trap_t;

extern void trap_construct(trap_t *trap, const char *name);
extern void trap_reset(trap_t *trap);
extern void trap_clock(trap_t *trap);
extern void trap_free(trap_t *trap);

#endif
