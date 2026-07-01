#include "hart.h"
#include "dbg/vcd.h"
#include "dbg/chk.h"
#include "spec/core/hart.h"

void hart_construct(hart_t *s, const char *name, const hart_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    EX_REQ_IF_CONSTRUCT(s, ex_req_itf, 1);
    EX_RSP_IF_CONSTRUCT(s, ex_rsp_itf, 1);
    FL_REQ_IF_CONSTRUCT(s, fl_req_itf, 1);
    FCH_REQ_IF_CONSTRUCT(s, fch_req_itf, 1);
    FCH_RSP_IF_CONSTRUCT(s, fch_rsp_itf, 1);
    LDST_REQ_IF_CONSTRUCT(s, ldst_req_itf, 1);
    LDST_RSP_IF_CONSTRUCT(s, ldst_rsp_itf, 1);
    BTI_IF_CONSTRUCT(s, va_i_, 1);
    BTI_IF_CONSTRUCT(s, va_d_, 1);
    BTI_IF_CONSTRUCT(s, pa_i_, 1);
    BTI_IF_CONSTRUCT(s, pa_d_, 1);
    BTI_IF_CONSTRUCT(s, pa_ptw_, 1);
    BTI_IF_CONSTRUCT(s, l1d_, 1);
    TLB_FLUSH_IF_CONSTRUCT(s, tlb_flush_itf, 1);
    L1_FLUSH_IF_CONSTRUCT(s, l1i_flush_itf, 1);
    HART_EXPT_IF_CONSTRUCT(s, i_hart_expt_itf, 1);
    HART_EXPT_IF_CONSTRUCT(s, hart_expt_itf, 1);
    TRAP_SEND_IF_CONSTRUCT(s, trap_send_itf, 1);
    EXU_CSR_READ_REQ_SIGNAL_IF_CONSTRUCT(s, exu_csr_read_req_sig_itf, false, false);
    CSR_EXU_READ_RSP_SIGNAL_IF_CONSTRUCT(s, csr_exu_read_rsp_sig_itf, false, false);
    EXU_CSR_WRITE_REQ_SIGNAL_IF_CONSTRUCT(s, exu_csr_write_req_sig_itf, false, false);
    CSR_EXU_WRITE_RSP_SIGNAL_IF_CONSTRUCT(s, csr_exu_write_rsp_sig_itf, false, false);

    s->ifu.fch_req_mst = &s->fch_req_itf;
    s->ifu.fch_rsp_slv = &s->fch_rsp_itf;
    s->ifu.ex_req_mst = &s->ex_req_itf;
    s->ifu.ex_rsp_slv = &s->ex_rsp_itf;
    s->ifu.fl_req_mst = &s->fl_req_itf;
    s->ifu.hart_expt_mst = &s->hart_expt_itf;
    s->ifu.trap_send_slv = &s->trap_send_itf;
    ifu_construct(&s->ifu, "u_ifu", conf->boot_rom_base, conf->boot_rom_base, conf->boot_rom_size);

    s->exu.fl_req_slv = &s->fl_req_itf;
    s->exu.ex_req_slv = &s->ex_req_itf;
    s->exu.ex_rsp_mst = &s->ex_rsp_itf;
    s->exu.ldst_req_mst = &s->ldst_req_itf;
    s->exu.ldst_rsp_slv = &s->ldst_rsp_itf;
    s->exu.hart_expt_mst = &s->hart_expt_itf;
    s->exu.exu_csr_read_req_out = &s->exu_csr_read_req_sig_itf;
    s->exu.csr_exu_read_rsp_in = &s->csr_exu_read_rsp_sig_itf;
    s->exu.exu_csr_write_req_out = &s->exu_csr_write_req_sig_itf;
    s->exu.csr_exu_write_rsp_in = &s->csr_exu_write_rsp_sig_itf;
    s->exu.tlb_flush_mst = &s->tlb_flush_itf;
    s->exu.l1i_flush_mst = &s->l1i_flush_itf;
    exu_construct(&s->exu, "u_exu");

    s->csr.cycle = s->cycle;
    s->csr.core_s_irq_slv = s->core_s_irq_slv;
    s->csr.core_timer_in = s->core_timer_in;
    s->csr.core_m_irq_in = s->core_m_irq_in;
    s->csr.core_swi_pend_out = s->core_swi_pend_out;
    s->csr.exu_csr_read_req_in = &s->exu_csr_read_req_sig_itf;
    s->csr.csr_exu_read_rsp_out = &s->csr_exu_read_rsp_sig_itf;
    s->csr.exu_csr_write_req_in = &s->exu_csr_write_req_sig_itf;
    s->csr.csr_exu_write_rsp_out = &s->csr_exu_write_rsp_sig_itf;
    csr_construct(&s->csr, "u_csr");

    BTI_MST_CONNECT(&s->hbi, i_, s, va_i_);
    BTI_MST_CONNECT(&s->hbi, d_, s, va_d_);
    s->hbi.fch_req_slv = &s->fch_req_itf;
    s->hbi.fch_rsp_mst = &s->fch_rsp_itf;
    s->hbi.i_hart_expt_slv = &s->i_hart_expt_itf;
    s->hbi.ldst_req_slv = &s->ldst_req_itf;
    s->hbi.ldst_rsp_mst = &s->ldst_rsp_itf;
    hbi_construct(&s->hbi, "u_hbi");

    BTI_SLV_CONNECT(&s->mmu, va_i_, s, va_i_);
    BTI_SLV_CONNECT(&s->mmu, va_d_, s, va_d_);
    s->mmu.i_hart_expt_mst = &s->i_hart_expt_itf;
    s->mmu.hart_expt_mst = &s->hart_expt_itf;
    BTI_MST_CONNECT(&s->mmu, pa_i_, s, pa_i_);
    BTI_MST_CONNECT(&s->mmu, pa_d_, s, pa_d_);
    BTI_MST_CONNECT(&s->mmu, ptw_, s, pa_ptw_);
    s->mmu.tlb_flush_slv = &s->tlb_flush_itf;
    s->mmu.priv = &s->exu.priv;
    s->mmu.satp = &s->csr.regs.satp;
    s->mmu.mstatus = &s->csr.regs.mstatus;
    s->mmu.ifu_pc = &s->ifu.fch.pc;
    s->mmu.exu_pc = &s->exu.cur_pc;
    mmu_conf_t mmu_conf = {};
    mmu_construct(&s->mmu, "u_mmu", &mmu_conf);

    BTI_SLV_ARR_CONNECT(&s->l1d_bti_mux, host_, 0, s, pa_d_);
    BTI_SLV_ARR_CONNECT(&s->l1d_bti_mux, host_, 1, s, pa_ptw_);
    BTI_MST_CONNECT(&s->l1d_bti_mux, gst_, s, l1d_);
    bti_mux_construct(&s->l1d_bti_mux, "u_l1d_bti_mux", 2);

    l1_conf_t l1i_conf = {
        .full_bypass = false,
        .ro = true,
        .size = L1I_SIZE,
        .way_num = L1I_WAY_NUM,
        .bypass_bases = { conf->boot_rom_base, conf->itcm_base },
        .bypass_sizes = { conf->boot_rom_size, conf->itcm_size }
    };
    s->l1i.cycle = s->cycle;
    BTI_SLV_CONNECT(&s->l1i, , s, pa_i_);
    AXI4_MST_IMPORT(&s->l1i, , s, i_);
    s->l1i.flush_slv = &s->l1i_flush_itf;
    l1_construct(&s->l1i, "u_l1i", &l1i_conf);

    l1_conf_t l1d_conf = {
        .full_bypass = true,
        .ro = false,
        .size = L1D_SIZE,
        .way_num = L1D_WAY_NUM,
        .bypass_bases = { conf->boot_rom_base, conf->itcm_base, conf->dtcm_base, conf->cfg_base },
        .bypass_sizes = { conf->boot_rom_size, conf->itcm_size, conf->dtcm_size, conf->cfg_size }
    };
    s->l1d.cycle = s->cycle;
    BTI_SLV_CONNECT(&s->l1d, , s, l1d_);
    AXI4_MST_IMPORT(&s->l1d, , s, d_);
    s->l1d.flush_slv = NULL;
    l1_construct(&s->l1d, "u_l1d", &l1d_conf);

    s->trap.hart_expt_slv = &s->hart_expt_itf;
    s->trap.trap_send_mst = &s->trap_send_itf;
    s->trap.core_m_irq_in = s->core_m_irq_in;
    s->trap.ext_irq_in = s->ext_irq_in;
    s->trap.priv = &s->exu.priv;
    s->trap.mstatus = &s->csr.regs.mstatus;
    s->trap.mcause = &s->csr.regs.mcause;
    s->trap.mip = &s->csr.regs.mip;
    s->trap.mie = &s->csr.regs.mie;
    s->trap.mtvec = &s->csr.regs.mtvec;
    s->trap.mepc = &s->csr.regs.mepc;
    s->trap.mtval = &s->csr.regs.mtval;
    s->trap.medeleg = &s->csr.regs.medeleg;
    s->trap.mideleg = &s->csr.regs.mideleg;
    s->trap.sstatus = &s->csr.regs.sstatus;
    s->trap.scause = &s->csr.regs.scause;
    s->trap.sip = &s->csr.regs.sip;
    s->trap.sie = &s->csr.regs.sie;
    s->trap.stvec = &s->csr.regs.stvec;
    s->trap.sepc = &s->csr.regs.sepc;
    s->trap.stval = &s->csr.regs.stval;
    s->trap.irq_epc = &s->exu.irq_epc;
    s->trap.irq_defer = &s->exu.irq_defer;
    s->trap.exu_wfi = &s->exu.wfi;
    s->trap.exu_wfi_resume_pc = &s->exu.wfi_resume_pc;
    trap_construct(&s->trap, "u_trap");
}

void hart_reset(hart_t *s)
{
    ifu_reset(&s->ifu);
    exu_reset(&s->exu);
    csr_reset(&s->csr);
    hbi_reset(&s->hbi);
    mmu_reset(&s->mmu);
    bti_mux_reset(&s->l1d_bti_mux);
    l1_reset(&s->l1i);
    l1_reset(&s->l1d);
    trap_reset(&s->trap);

    itf_reset(&s->ex_req_itf);
    itf_reset(&s->ex_rsp_itf);
    itf_reset(&s->fl_req_itf);
    itf_reset(&s->fch_req_itf);
    itf_reset(&s->fch_rsp_itf);
    itf_reset(&s->ldst_req_itf);
    itf_reset(&s->ldst_rsp_itf);
    BTI_IF_RESET(s, va_i_);
    BTI_IF_RESET(s, va_d_);
    BTI_IF_RESET(s, pa_i_);
    BTI_IF_RESET(s, pa_d_);
    BTI_IF_RESET(s, pa_ptw_);
    BTI_IF_RESET(s, l1d_);
    itf_reset(&s->tlb_flush_itf);
    itf_reset(&s->l1i_flush_itf);
    itf_reset(&s->i_hart_expt_itf);
    itf_reset(&s->hart_expt_itf);
    itf_reset(&s->trap_send_itf);
    itf_reset(&s->exu_csr_read_req_sig_itf);
    itf_reset(&s->csr_exu_read_rsp_sig_itf);
    itf_reset(&s->exu_csr_write_req_sig_itf);
    itf_reset(&s->csr_exu_write_rsp_sig_itf);
}

void hart_free(hart_t *s)
{
    ifu_free(&s->ifu);
    exu_free(&s->exu);
    csr_free(&s->csr);
    hbi_free(&s->hbi);
    mmu_free(&s->mmu);
    bti_mux_free(&s->l1d_bti_mux);
    l1_free(&s->l1i);
    l1_free(&s->l1d);
    trap_free(&s->trap);

    itf_free(&s->ex_req_itf);
    itf_free(&s->ex_rsp_itf);
    itf_free(&s->fl_req_itf);
    itf_free(&s->fch_req_itf);
    itf_free(&s->fch_rsp_itf);
    itf_free(&s->ldst_req_itf);
    itf_free(&s->ldst_rsp_itf);
    BTI_IF_FREE(s, va_i_);
    BTI_IF_FREE(s, va_d_);
    BTI_IF_FREE(s, pa_i_);
    BTI_IF_FREE(s, pa_d_);
    BTI_IF_FREE(s, pa_ptw_);
    BTI_IF_FREE(s, l1d_);
    itf_free(&s->tlb_flush_itf);
    itf_free(&s->l1i_flush_itf);
    itf_free(&s->i_hart_expt_itf);
    itf_free(&s->hart_expt_itf);
    itf_free(&s->trap_send_itf);
    itf_free(&s->exu_csr_read_req_sig_itf);
    itf_free(&s->csr_exu_read_rsp_sig_itf);
    itf_free(&s->exu_csr_write_req_sig_itf);
    itf_free(&s->csr_exu_write_rsp_sig_itf);
}

void hart_clock(hart_t *s)
{
    exu_clock(&s->exu);
    csr_clock(&s->csr);
    trap_clock(&s->trap);
    ifu_clock(&s->ifu);
    hbi_clock(&s->hbi);
    mmu_clock(&s->mmu);
    bti_mux_clock(&s->l1d_bti_mux);
    l1_clock(&s->l1i);
    l1_clock(&s->l1d);

    itf_dbg_clock(&s->ex_req_itf);
    itf_dbg_clock(&s->ex_rsp_itf);
    itf_dbg_clock(&s->fl_req_itf);
    itf_dbg_clock(&s->fch_req_itf);
    itf_dbg_clock(&s->fch_rsp_itf);
    itf_dbg_clock(&s->ldst_req_itf);
    itf_dbg_clock(&s->ldst_rsp_itf);
    BTI_IF_DBG_CLOCK(s, va_i_);
    BTI_IF_DBG_CLOCK(s, va_d_);
    BTI_IF_DBG_CLOCK(s, pa_i_);
    BTI_IF_DBG_CLOCK(s, pa_d_);
    BTI_IF_DBG_CLOCK(s, pa_ptw_);
    BTI_IF_DBG_CLOCK(s, l1d_);
    itf_dbg_clock(&s->tlb_flush_itf);
    itf_dbg_clock(&s->l1i_flush_itf);
    itf_dbg_clock(&s->i_hart_expt_itf);
    itf_dbg_clock(&s->hart_expt_itf);
    itf_dbg_clock(&s->trap_send_itf);
    itf_dbg_clock(&s->exu_csr_read_req_sig_itf);
    itf_dbg_clock(&s->csr_exu_read_rsp_sig_itf);
    itf_dbg_clock(&s->exu_csr_write_req_sig_itf);
    itf_dbg_clock(&s->csr_exu_write_rsp_sig_itf);
}
