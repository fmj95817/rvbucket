#include "csr.h"
#include "dbg/chk.h"

#define CSR_S_IRQ_MASK ( \
    RV32G_CSR_MIP_SSIP_BIT | RV32G_CSR_MIP_STIP_BIT | \
    RV32G_CSR_MIP_SEIP_BIT | RV32G_CSR_MIP_LCOFIP_BIT)

static void sync_sstatus_from_mstatus(csr_t *csr)
{
    csr->regs.sstatus.reg.sie = csr->regs.mstatus.reg.sie;
    csr->regs.sstatus.reg.spie = csr->regs.mstatus.reg.spie;
    csr->regs.sstatus.reg.ube = csr->regs.mstatus.reg.ube;
    csr->regs.sstatus.reg.spp = csr->regs.mstatus.reg.spp;
    csr->regs.sstatus.reg.vs = csr->regs.mstatus.reg.vs;
    csr->regs.sstatus.reg.fs = csr->regs.mstatus.reg.fs;
    csr->regs.sstatus.reg.xs = csr->regs.mstatus.reg.xs;
    csr->regs.sstatus.reg.sum = csr->regs.mstatus.reg.sum;
    csr->regs.sstatus.reg.mxr = csr->regs.mstatus.reg.mxr;
    csr->regs.sstatus.reg.spelp = csr->regs.mstatus.reg.spelp;
    csr->regs.sstatus.reg.sd = csr->regs.mstatus.reg.sd;
}

static void sync_mstatus_from_sstatus(csr_t *csr)
{
    csr->regs.mstatus.reg.sie = csr->regs.sstatus.reg.sie;
    csr->regs.mstatus.reg.spie = csr->regs.sstatus.reg.spie;
    csr->regs.mstatus.reg.ube = csr->regs.sstatus.reg.ube;
    csr->regs.mstatus.reg.spp = csr->regs.sstatus.reg.spp;
    csr->regs.mstatus.reg.vs = csr->regs.sstatus.reg.vs;
    csr->regs.mstatus.reg.fs = csr->regs.sstatus.reg.fs;
    csr->regs.mstatus.reg.xs = csr->regs.sstatus.reg.xs;
    csr->regs.mstatus.reg.sum = csr->regs.sstatus.reg.sum;
    csr->regs.mstatus.reg.mxr = csr->regs.sstatus.reg.mxr;
    csr->regs.mstatus.reg.spelp = csr->regs.sstatus.reg.spelp;
    csr->regs.mstatus.reg.sd = csr->regs.sstatus.reg.sd;
}

static void sync_s_irq_from_m_irq(csr_t *csr)
{
    csr->regs.sie.raw = csr->regs.mie.raw & CSR_S_IRQ_MASK;
    csr->regs.sip.raw = (csr->regs.sip.raw & ~CSR_S_IRQ_MASK) |
        (csr->regs.mip.raw & CSR_S_IRQ_MASK);
}

static void sync_m_irq_from_s_irq(csr_t *csr)
{
    csr->regs.mie.raw = (csr->regs.mie.raw & ~CSR_S_IRQ_MASK) |
        (csr->regs.sie.raw & CSR_S_IRQ_MASK);
    csr->regs.mip.raw = (csr->regs.mip.raw & ~CSR_S_IRQ_MASK) |
        (csr->regs.sip.raw & CSR_S_IRQ_MASK);
}

static void csr_sync_supervisor_aliases(csr_t *csr, u32 written_addr)
{
    switch (written_addr) {
    case 0x100:
        sync_mstatus_from_sstatus(csr);
        break;
    case 0x104:
    case 0x144:
        sync_m_irq_from_s_irq(csr);
        break;
    case 0x300:
        sync_sstatus_from_mstatus(csr);
        break;
    case 0x304:
    case 0x344:
        sync_s_irq_from_m_irq(csr);
        break;
    default:
        break;
    }
}

static void m_irq_cb(void *args)
{
    DBG_CHECK(args != NULL);
    csr_t *csr = args;

    csr->regs.mip.reg.mtip = csr->core_m_irq_i->mtimer ? 1u : 0u;
    csr->regs.mip.reg.msip = csr->core_m_irq_i->msw ? 1u : 0u;
    sync_s_irq_from_m_irq(csr);
}

static void csr_read_cb(void *args)
{
    DBG_CHECK(args != NULL);
    csr_t *csr = args;

    u32 val;
    bool ok = rv32g_csr_read(&csr->regs,
        csr->read_req_i->priv, csr->read_req_i->addr, &val);

    csr->read_rsp_o->val = val;
    csr->read_rsp_o->ok = ok;
    itf_signal_write_notify(csr->csr_exu_read_rsp_out);
}

static void csr_write_cb(void *args)
{
    DBG_CHECK(args != NULL);
    csr_t *csr = args;

    csr->write_rsp_o->ok = rv32g_csr_write(&csr->regs,
        csr->write_req_i->priv, csr->write_req_i->addr, csr->write_req_i->val);
    if (csr->write_rsp_o->ok) {
        csr_sync_supervisor_aliases(csr, csr->write_req_i->addr);
    }
    itf_signal_write_notify(csr->csr_exu_write_rsp_out);

    csr->core_swi_pend_o->msip = (csr->regs.mip.reg.msip != 0u);
}

void csr_construct(csr_t *csr, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    csr->core_timer_i = itf_signal_get_src_and_chk(csr->core_timer_in);
    csr->core_m_irq_i = itf_signal_get_src_and_chk(csr->core_m_irq_in);
    csr->core_swi_pend_o = itf_signal_get_src_and_chk(csr->core_swi_pend_out);
    csr->read_req_i = itf_signal_get_src_and_chk(csr->exu_csr_read_req_in);
    csr->read_rsp_o = itf_signal_get_src_and_chk(csr->csr_exu_read_rsp_out);
    csr->write_req_i = itf_signal_get_src_and_chk(csr->exu_csr_write_req_in);
    csr->write_rsp_o = itf_signal_get_src_and_chk(csr->csr_exu_write_rsp_out);

    itf_signal_set_wcb(csr->exu_csr_read_req_in, &csr_read_cb, csr);
    itf_signal_set_wcb(csr->exu_csr_write_req_in, &csr_write_cb, csr);
    itf_signal_set_wcb(csr->core_m_irq_in, &m_irq_cb, csr);
}

void csr_reset(csr_t *csr)
{
    rv32g_csr_reset(&csr->regs);
    csr->regs.stimecmp = 0xffffffffu;
    csr->regs.stimecmph = 0xffffffffu;
    sync_sstatus_from_mstatus(csr);
    sync_s_irq_from_m_irq(csr);
    csr->core_swi_pend_o->msip = (csr->regs.mip.reg.msip != 0u);
}

static void csr_update_proc(csr_t *csr)
{
    csr->regs.time = (u32)(csr->core_timer_i->time);
    csr->regs.timeh = (u32)(csr->core_timer_i->time >> 32u);

    u64 time = ((u64)csr->regs.timeh << 32u) | csr->regs.time;
    u64 stimecmp = ((u64)csr->regs.stimecmph << 32u) | csr->regs.stimecmp;
    csr->regs.mip.reg.stip = (time >= stimecmp) ? 1u : 0u;
    sync_s_irq_from_m_irq(csr);

    csr->regs.cycle = (u32)(*csr->cycle);
    csr->regs.cycleh = (u32)(*csr->cycle >> 32u);
}

static void csr_core_s_irq_proc(csr_t *csr)
{
    DBG_CHECK(csr->core_s_irq_slv != NULL);
    if (itf_fifo_empty(csr->core_s_irq_slv)) {
        return;
    }

    core_s_irq_if_t core_s_irq;
    itf_read(csr->core_s_irq_slv, &core_s_irq);
    if (core_s_irq.ssw) {
        csr->regs.mip.reg.ssip = 1u;
        csr->regs.sip.reg.ssip = 1u;
        sync_m_irq_from_s_irq(csr);
    }
}

void csr_clock(csr_t *csr)
{
    csr_update_proc(csr);
    csr_core_s_irq_proc(csr);
}

void csr_free(csr_t *csr)
{
}
