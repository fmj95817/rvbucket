#include "trap.h"
#include "itf/fl_req_if.h"
#include "dbg/log.h"
#include "dbg/chk.h"

#define EPC_ALIGN_MASK (~0x3u)

static void trap_send_redirect(trap_t *trap, u32 target_pc)
{
    trap_send_if_t pkt;
    pkt.target_pc = target_pc;
    itf_write(trap->trap_send_mst, &pkt);

    if (!itf_fifo_full(trap->fl_req_mst)) {
        fl_req_if_t fl_req = {};
        itf_write(trap->fl_req_mst, &fl_req);
    }
}

static u32 trap_compute_tvec_pc(const rv32g_csr_mtvec_t *tvec, bool is_interrupt, u32 cause)
{
    u32 base = tvec->reg.base << 2;
    if (is_interrupt && tvec->reg.mode == RV32G_CSR_MTVEC_MODE_VECTORED) {
        return base + cause * 4;
    }
    return base;
}

static u32 trap_compute_stvec_pc(const rv32g_csr_stvec_t *stvec, bool is_interrupt, u32 cause)
{
    u32 base = stvec->reg.base << 2;
    if (is_interrupt && stvec->reg.mode == RV32G_CSR_STVEC_MODE_VECTORED) {
        return base + cause * 4;
    }
    return base;
}

static bool trap_is_delegated_to_s(trap_t *trap, bool is_interrupt, u32 cause)
{
    if (*trap->priv == RV32G_PRIV_MACHINE) {
        return false;
    }
    u32 deleg = is_interrupt ? trap->mideleg->raw : trap->medeleg->raw;
    return (deleg >> cause) & 1;
}

static void trap_enter_m_mode(trap_t *trap, u32 cause, bool is_interrupt, u32 epc, u32 tval)
{
    trap->mstatus->reg.mpie = trap->mstatus->reg.mie;
    trap->mstatus->reg.mpp = *trap->priv;
    trap->mstatus->reg.mie = 0;

    trap->mcause->reg.code = cause;
    trap->mcause->reg.interrupt = is_interrupt ? 1 : 0;

    *trap->mepc = epc & EPC_ALIGN_MASK;
    *trap->mtval = tval;
    *trap->priv = RV32G_PRIV_MACHINE;

    u32 target_pc = trap_compute_tvec_pc(trap->mtvec, is_interrupt, cause);
    trap_send_redirect(trap, target_pc);

    DBG_LOG(LOG_TRACE, "trap: enter M-mode, cause=%u, int=%u, epc=%08x, tval=%08x, pc->%08x\n",
        cause, is_interrupt, epc, tval, target_pc);
}

static void trap_enter_s_mode(trap_t *trap, u32 cause, bool is_interrupt, u32 epc, u32 tval)
{
    trap->mstatus->reg.spie = trap->mstatus->reg.sie;
    trap->mstatus->reg.spp = (*trap->priv == RV32G_PRIV_SUPERVISOR) ? 1 : 0;
    trap->mstatus->reg.sie = 0;

    trap->sstatus->reg.spie = trap->mstatus->reg.spie;
    trap->sstatus->reg.spp = trap->mstatus->reg.spp;
    trap->sstatus->reg.sie = 0;

    trap->scause->reg.code = cause;
    trap->scause->reg.interrupt = is_interrupt ? 1 : 0;

    *trap->sepc = epc & EPC_ALIGN_MASK;
    *trap->stval = tval;
    *trap->priv = RV32G_PRIV_SUPERVISOR;

    u32 target_pc = trap_compute_stvec_pc(trap->stvec, is_interrupt, cause);
    trap_send_redirect(trap, target_pc);

    DBG_LOG(LOG_TRACE, "trap: enter S-mode, cause=%u, int=%u, epc=%08x, tval=%08x, pc->%08x\n",
        cause, is_interrupt, epc, tval, target_pc);
}

static void trap_take(trap_t *trap, u32 cause, bool is_interrupt, u32 epc, u32 tval)
{
    if (trap_is_delegated_to_s(trap, is_interrupt, cause)) {
        trap_enter_s_mode(trap, cause, is_interrupt, epc, tval);
    } else {
        trap_enter_m_mode(trap, cause, is_interrupt, epc, tval);
    }
    *trap->exu_wfi = false;
}

static void trap_do_mret(trap_t *trap)
{
    u32 prev_priv = trap->mstatus->reg.mpp;
    trap->mstatus->reg.mie = trap->mstatus->reg.mpie;
    trap->mstatus->reg.mpie = 1;
    trap->mstatus->reg.mpp = RV32G_PRIV_USER;

    if (prev_priv != RV32G_PRIV_MACHINE) {
        trap->mstatus->reg.mprv = 0;
    }

    *trap->priv = prev_priv;

    u32 target_pc = *trap->mepc & EPC_ALIGN_MASK;
    trap_send_redirect(trap, target_pc);

    DBG_LOG(LOG_TRACE, "trap: mret, pc->%08x, priv->%u\n", target_pc, prev_priv);
}

static void trap_do_sret(trap_t *trap)
{
    u32 prev_priv = trap->mstatus->reg.spp;
    trap->mstatus->reg.sie = trap->mstatus->reg.spie;
    trap->mstatus->reg.spie = 1;
    trap->mstatus->reg.spp = 0;

    trap->sstatus->reg.sie = trap->mstatus->reg.sie;
    trap->sstatus->reg.spie = 1;
    trap->sstatus->reg.spp = 0;

    trap->mstatus->reg.mprv = 0;

    *trap->priv = prev_priv;

    u32 target_pc = *trap->sepc & EPC_ALIGN_MASK;
    trap_send_redirect(trap, target_pc);

    DBG_LOG(LOG_TRACE, "trap: sret, pc->%08x, priv->%u\n", target_pc, prev_priv);
}

static void trap_proc_exception(trap_t *trap)
{
    if (itf_fifo_empty(trap->hart_expt_slv)) {
        return;
    }

    hart_expt_if_t expt;
    itf_read(trap->hart_expt_slv, &expt);

    switch (expt.type) {
    case HART_EXPT_TYPE_MRET:
        trap_do_mret(trap);
        break;
    case HART_EXPT_TYPE_SRET:
        trap_do_sret(trap);
        break;
    case HART_EXPT_TYPE_EXCEPTION: {
        u32 cause = (u32)expt.cause;
        if (expt.cause == HART_EXPT_CAUSE_ECALL) {
            switch (*trap->priv) {
            case RV32G_PRIV_USER:
                cause = RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_U;
                break;
            case RV32G_PRIV_SUPERVISOR:
                cause = RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_S;
                break;
            case RV32G_PRIV_MACHINE:
                cause = RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_M;
                break;
            default:
                DBG_CHECK(0);
                break;
            }
        }
        trap_take(trap, cause, false, expt.pc, expt.tval);
        break;
    }
    default:
        DBG_CHECK(0);
        break;
    }
}

static int trap_pending_irq(trap_t *trap)
{
    u32 pending = trap->mip->raw & trap->mie->raw;
    if (pending == 0) {
        return -1;
    }

    rv32g_priv_t priv = *trap->priv;
    bool m_ie = (priv == RV32G_PRIV_MACHINE) ? (trap->mstatus->reg.mie != 0) : true;
    bool s_ie = (priv == RV32G_PRIV_SUPERVISOR) ? (trap->mstatus->reg.sie != 0) : (priv < RV32G_PRIV_SUPERVISOR);

    u32 m_enabled = pending & ~trap->mideleg->raw;
    u32 s_enabled = pending & trap->mideleg->raw;

    if (m_ie && m_enabled) {
        if (m_enabled & RV32G_CSR_MIP_MEIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_M_MODE_EXT;
        if (m_enabled & RV32G_CSR_MIP_MSIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_M_MODE_SW;
        if (m_enabled & RV32G_CSR_MIP_MTIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_M_MODE_TIMER;
        if (m_enabled & RV32G_CSR_MIP_SEIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_S_MODE_EXT;
        if (m_enabled & RV32G_CSR_MIP_SSIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_S_MODE_SW;
        if (m_enabled & RV32G_CSR_MIP_STIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_S_MODE_TIMER;
        if (m_enabled & RV32G_CSR_MIP_LCOFIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_COUNTER_OVERFLOW;
    }

    if (s_ie && s_enabled) {
        if (s_enabled & RV32G_CSR_MIP_SEIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_S_MODE_EXT;
        if (s_enabled & RV32G_CSR_MIP_SSIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_S_MODE_SW;
        if (s_enabled & RV32G_CSR_MIP_STIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_S_MODE_TIMER;
        if (s_enabled & RV32G_CSR_MIP_LCOFIP_BIT) return (int)RV32G_CSR_MCAUSE_INTR_COUNTER_OVERFLOW;
    }

    return -1;
}

static void trap_proc_interrupt(trap_t *trap)
{
    int irq = trap_pending_irq(trap);
    if (irq < 0) {
        return;
    }

    bool from_wfi = *trap->exu_wfi;
    u32 epc = from_wfi ? *trap->exu_wfi_resume_pc : *trap->ifu_pc;
    *trap->exu_wfi = false;
    trap_take(trap, (u32)irq, true, epc, 0);

    (void)from_wfi;
}

void trap_construct(trap_t *trap, const char *name)
{
    (void)name;
    trap->core_m_irq_i = itf_signal_get_src_and_chk(trap->core_m_irq_in);
    trap->ext_irq_i = itf_signal_get_src_and_chk(trap->ext_irq_in);
}

void trap_reset(trap_t *trap)
{
    (void)trap;
}

void trap_clock(trap_t *trap)
{
    trap->mip->reg.meip = trap->ext_irq_i->irq ? 1u : 0u;

    if (!itf_fifo_empty(trap->hart_expt_slv)) {
        trap_proc_exception(trap);
        return;
    }
    trap_proc_interrupt(trap);
}

void trap_free(trap_t *trap)
{
    (void)trap;
}
