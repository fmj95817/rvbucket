#include "trap.h"
#include "dbg/log.h"
#include "dbg/chk.h"

#define EPC_ALIGN_MASK (~0x3u)

#define CSR_SSTATUS  0x100u
#define CSR_SEPC     0x141u
#define CSR_SCAUSE   0x142u
#define CSR_STVAL    0x143u
#define CSR_MSTATUS  0x300u
#define CSR_MEPC     0x341u
#define CSR_MCAUSE   0x342u
#define CSR_MTVAL    0x343u

static rv32g_csr_mstatus_t trap_mstatus(const trap_t *trap)
{
    return (rv32g_csr_mstatus_t){ .raw = trap->csr_state_i->mstatus };
}

static void trap_write_csr(trap_t *trap, u32 addr, u32 val)
{
    trap->csr_write_req_o->addr = addr;
    trap->csr_write_req_o->val = val;
    itf_signal_write_notify(trap->trap_csr_write_req_out);
    DBG_CHECK(trap->csr_write_rsp_i->ok);
}

static void trap_update_exu(trap_t *trap, rv32g_priv_t priv, u32 irq_epc, bool wfi)
{
    trap->exu_ctrl_o->priv = priv;
    trap->exu_ctrl_o->irq_epc = irq_epc;
    trap->exu_ctrl_o->wfi = wfi;
    itf_signal_write_notify(trap->trap_exu_ctrl_out);
}

static void trap_send_redirect(trap_t *trap, u32 target_pc)
{
    trap_send_if_t pkt = { .target_pc = target_pc };
    itf_write(trap->trap_send_mst, &pkt);
}

static u32 trap_compute_tvec_pc(u32 raw, bool is_interrupt, u32 cause)
{
    rv32g_csr_mtvec_t tvec = { .raw = raw };
    u32 base = tvec.reg.base << 2;
    if (is_interrupt && tvec.reg.mode == RV32G_CSR_MTVEC_MODE_VECTORED) {
        return base + cause * 4;
    }
    return base;
}

static u32 trap_compute_stvec_pc(u32 raw, bool is_interrupt, u32 cause)
{
    rv32g_csr_stvec_t stvec = { .raw = raw };
    u32 base = stvec.reg.base << 2;
    if (is_interrupt && stvec.reg.mode == RV32G_CSR_STVEC_MODE_VECTORED) {
        return base + cause * 4;
    }
    return base;
}

static bool trap_is_delegated_to_s(const trap_t *trap, rv32g_priv_t src_priv,
    bool is_interrupt, u32 cause)
{
    if (src_priv == RV32G_PRIV_MACHINE) {
        return false;
    }
    u32 deleg = is_interrupt ? trap->csr_state_i->mideleg : trap->csr_state_i->medeleg;
    return ((deleg >> cause) & 1u) != 0;
}

static void trap_enter_m_mode(trap_t *trap, rv32g_priv_t src_priv, u32 cause,
    bool is_interrupt, u32 epc, u32 tval)
{
    rv32g_csr_mstatus_t mstatus = trap_mstatus(trap);
    rv32g_csr_mcause_t mcause = {};
    mstatus.reg.mpie = mstatus.reg.mie;
    mstatus.reg.mpp = src_priv;
    mstatus.reg.mie = 0;
    mcause.reg.code = cause;
    mcause.reg.interrupt = is_interrupt ? 1u : 0u;

    trap_write_csr(trap, CSR_MSTATUS, mstatus.raw);
    trap_write_csr(trap, CSR_MCAUSE, mcause.raw);
    trap_write_csr(trap, CSR_MEPC, epc & EPC_ALIGN_MASK);
    trap_write_csr(trap, CSR_MTVAL, tval);

    u32 target_pc = trap_compute_tvec_pc(trap->csr_state_i->mtvec, is_interrupt, cause);
    trap_update_exu(trap, RV32G_PRIV_MACHINE, target_pc, false);
    trap_send_redirect(trap, target_pc);

    DBG_LOG(LOG_TRAP, "trap: enter M-mode, cause=%u, int=%u, epc=%08x, tval=%08x, pc->%08x\n",
        cause, is_interrupt, epc, tval, target_pc);
}

static void trap_enter_s_mode(trap_t *trap, rv32g_priv_t src_priv, u32 cause,
    bool is_interrupt, u32 epc, u32 tval)
{
    rv32g_csr_mstatus_t mstatus = trap_mstatus(trap);
    rv32g_csr_scause_t scause = {};
    mstatus.reg.spie = mstatus.reg.sie;
    mstatus.reg.spp = (src_priv == RV32G_PRIV_SUPERVISOR) ? 1u : 0u;
    mstatus.reg.sie = 0;
    scause.reg.code = cause;
    scause.reg.interrupt = is_interrupt ? 1u : 0u;

    trap_write_csr(trap, CSR_MSTATUS, mstatus.raw);
    trap_write_csr(trap, CSR_SCAUSE, scause.raw);
    trap_write_csr(trap, CSR_SEPC, epc & EPC_ALIGN_MASK);
    trap_write_csr(trap, CSR_STVAL, tval);

    u32 target_pc = trap_compute_stvec_pc(trap->csr_state_i->stvec, is_interrupt, cause);
    trap_update_exu(trap, RV32G_PRIV_SUPERVISOR, target_pc, false);
    trap_send_redirect(trap, target_pc);

    DBG_LOG(LOG_TRAP, "trap: enter S-mode, cause=%u, int=%u, epc=%08x, tval=%08x, pc->%08x\n",
        cause, is_interrupt, epc, tval, target_pc);
}

static void trap_take(trap_t *trap, rv32g_priv_t src_priv, u32 cause,
    bool is_interrupt, u32 epc, u32 tval)
{
    if (trap_is_delegated_to_s(trap, src_priv, is_interrupt, cause)) {
        trap_enter_s_mode(trap, src_priv, cause, is_interrupt, epc, tval);
    } else {
        trap_enter_m_mode(trap, src_priv, cause, is_interrupt, epc, tval);
    }
}

static void trap_do_mret(trap_t *trap)
{
    rv32g_csr_mstatus_t mstatus = trap_mstatus(trap);
    rv32g_priv_t prev_priv = (rv32g_priv_t)mstatus.reg.mpp;
    mstatus.reg.mie = mstatus.reg.mpie;
    mstatus.reg.mpie = 1;
    mstatus.reg.mpp = RV32G_PRIV_USER;
    if (prev_priv != RV32G_PRIV_MACHINE) {
        mstatus.reg.mprv = 0;
    }
    trap_write_csr(trap, CSR_MSTATUS, mstatus.raw);

    u32 target_pc = trap->csr_state_i->mepc & EPC_ALIGN_MASK;
    trap_update_exu(trap, prev_priv, target_pc, false);
    trap_send_redirect(trap, target_pc);
    DBG_LOG(LOG_TRAP, "trap: mret, pc->%08x, priv->%u\n", target_pc, prev_priv);
}

static void trap_do_sret(trap_t *trap)
{
    rv32g_csr_mstatus_t mstatus = trap_mstatus(trap);
    rv32g_priv_t prev_priv = mstatus.reg.spp ? RV32G_PRIV_SUPERVISOR : RV32G_PRIV_USER;
    mstatus.reg.sie = mstatus.reg.spie;
    mstatus.reg.spie = 1;
    mstatus.reg.spp = 0;
    mstatus.reg.mprv = 0;
    trap_write_csr(trap, CSR_MSTATUS, mstatus.raw);

    u32 target_pc = trap->csr_state_i->sepc & EPC_ALIGN_MASK;
    trap_update_exu(trap, prev_priv, target_pc, false);
    trap_send_redirect(trap, target_pc);
    DBG_LOG(LOG_TRAP, "trap: sret, pc->%08x, priv->%u\n", target_pc, prev_priv);
}

static void trap_handle_exception(trap_t *trap, const hart_expt_if_t *expt)
{
    switch (expt->type) {
    case HART_EXPT_TYPE_MRET:
        trap_do_mret(trap);
        return;
    case HART_EXPT_TYPE_SRET:
        trap_do_sret(trap);
        return;
    case HART_EXPT_TYPE_EXCEPTION:
        break;
    default:
        DBG_CHECK(0);
        return;
    }

    u32 cause = (u32)expt->cause;
    if (expt->cause == HART_EXPT_CAUSE_ECALL) {
        switch ((rv32g_priv_t)expt->priv) {
        case RV32G_PRIV_USER: cause = RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_U; break;
        case RV32G_PRIV_SUPERVISOR: cause = RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_S; break;
        case RV32G_PRIV_MACHINE: cause = RV32G_CSR_MCAUSE_EXPT_ECALL_FROM_M; break;
        default: DBG_CHECK(0); return;
        }
    }
    trap_take(trap, (rv32g_priv_t)expt->priv, cause, false, expt->pc, expt->tval);
}

static bool trap_proc_exception(trap_t *trap)
{
    itf_t *current_sources[] = { trap->ex_expt_slv, trap->ldst_expt_slv };
    for (u32 i = 0; i < sizeof(current_sources) / sizeof(current_sources[0]); i++) {
        itf_t *source = current_sources[i];
        if (!itf_fifo_empty(source)) {
            hart_expt_if_t expt;
            itf_read(source, &expt);
            trap_handle_exception(trap, &expt);
            return true;
        }
    }

    if (!trap->exu_state_i->trap_defer) {
        itf_t *source = trap->fch_expt_slv;
        if (!itf_fifo_empty(source)) {
            hart_expt_if_t expt;
            itf_read(source, &expt);
            trap_handle_exception(trap, &expt);
            return true;
        }
    }
    return false;
}

static int trap_pending_irq(const trap_t *trap)
{
    u32 pending = trap->csr_state_i->mip & trap->csr_state_i->mie;
    if (pending == 0) {
        return -1;
    }

    rv32g_csr_mstatus_t mstatus = trap_mstatus(trap);
    rv32g_priv_t priv = (rv32g_priv_t)trap->exu_state_i->priv;
    bool m_ie = (priv == RV32G_PRIV_MACHINE) ? (mstatus.reg.mie != 0) : true;
    bool s_ie = (priv == RV32G_PRIV_SUPERVISOR) ? (mstatus.reg.sie != 0) :
        (priv < RV32G_PRIV_SUPERVISOR);
    u32 m_enabled = pending & ~trap->csr_state_i->mideleg;
    u32 s_enabled = pending & trap->csr_state_i->mideleg;

    if (m_ie && m_enabled) {
        if (m_enabled & RV32G_CSR_MIP_MEIP_BIT) return RV32G_CSR_MCAUSE_INTR_M_MODE_EXT;
        if (m_enabled & RV32G_CSR_MIP_MSIP_BIT) return RV32G_CSR_MCAUSE_INTR_M_MODE_SW;
        if (m_enabled & RV32G_CSR_MIP_MTIP_BIT) return RV32G_CSR_MCAUSE_INTR_M_MODE_TIMER;
        if (m_enabled & RV32G_CSR_MIP_SEIP_BIT) return RV32G_CSR_MCAUSE_INTR_S_MODE_EXT;
        if (m_enabled & RV32G_CSR_MIP_SSIP_BIT) return RV32G_CSR_MCAUSE_INTR_S_MODE_SW;
        if (m_enabled & RV32G_CSR_MIP_STIP_BIT) return RV32G_CSR_MCAUSE_INTR_S_MODE_TIMER;
        if (m_enabled & RV32G_CSR_MIP_LCOFIP_BIT) return RV32G_CSR_MCAUSE_INTR_COUNTER_OVERFLOW;
    }
    if (s_ie && s_enabled) {
        if (s_enabled & RV32G_CSR_MIP_SEIP_BIT) return RV32G_CSR_MCAUSE_INTR_S_MODE_EXT;
        if (s_enabled & RV32G_CSR_MIP_SSIP_BIT) return RV32G_CSR_MCAUSE_INTR_S_MODE_SW;
        if (s_enabled & RV32G_CSR_MIP_STIP_BIT) return RV32G_CSR_MCAUSE_INTR_S_MODE_TIMER;
        if (s_enabled & RV32G_CSR_MIP_LCOFIP_BIT) return RV32G_CSR_MCAUSE_INTR_COUNTER_OVERFLOW;
    }
    return -1;
}

static void trap_proc_interrupt(trap_t *trap)
{
    if (trap->exu_state_i->trap_defer) {
        return;
    }

    int irq = trap_pending_irq(trap);
    bool wake = (trap->csr_state_i->mip & trap->csr_state_i->mie) != 0;
    if (irq < 0) {
        if (trap->exu_state_i->wfi && wake) {
            u32 epc = trap->exu_state_i->wfi_resume_pc;
            trap_update_exu(trap, (rv32g_priv_t)trap->exu_state_i->priv, epc, false);
            trap_send_redirect(trap, epc);
        }
        return;
    }

    u32 epc = trap->exu_state_i->wfi ? trap->exu_state_i->wfi_resume_pc :
        trap->exu_state_i->irq_epc;
    trap_take(trap, (rv32g_priv_t)trap->exu_state_i->priv, (u32)irq, true, epc, 0);
}

void trap_construct(trap_t *trap, const char *parent, const char *name)
{
    mod_construct(&trap->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    trap->exu_state_i = itf_signal_get_src_and_chk(trap->exu_state_in);
    trap->exu_ctrl_o = itf_signal_get_src_and_chk(trap->trap_exu_ctrl_out);
    trap->csr_state_i = itf_signal_get_src_and_chk(trap->csr_trap_state_in);
    trap->csr_write_req_o = itf_signal_get_src_and_chk(trap->trap_csr_write_req_out);
    trap->csr_write_rsp_i = itf_signal_get_src_and_chk(trap->csr_trap_write_rsp_in);
}

void trap_reset(trap_t *trap)
{
    mod_reset(&trap->mod);
    (void)trap;
}

void trap_clock(trap_t *trap)
{
    mod_clock(&trap->mod);
    if (!trap_proc_exception(trap)) {
        trap_proc_interrupt(trap);
    }
}

void trap_free(trap_t *trap)
{
    mod_free(&trap->mod);
    (void)trap;
}
