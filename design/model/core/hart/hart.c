#include "hart.h"
#include "dbg/vcd.h"
#include "dbg/chk.h"
#include "spec/core/hart.h"

void hart_construct(hart_t *s, const char *parent, const char *name, const hart_conf_t *conf)
{
    mod_construct(&s->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    EX_REQ_IF_CONSTRUCT(s, ex_req_itf, 1);
    EX_RSP_IF_CONSTRUCT(s, ex_rsp_itf, 1);
    FL_REQ_IF_CONSTRUCT(s, fl_req_itf, 1);
    FCH_REQ_IF_CONSTRUCT(s, fch_req_itf, 1);
    FCH_RSP_IF_CONSTRUCT(s, fch_rsp_itf, 1);
    LDST_REQ_IF_CONSTRUCT(s, exu_lsu_ldst_req_itf, 1);
    LDST_RSP_IF_CONSTRUCT(s, exu_lsu_ldst_rsp_itf, 1);
    LDST_REQ_IF_CONSTRUCT(s, lsu_hbi_ldst_req_itf, 1);
    LDST_RSP_IF_CONSTRUCT(s, lsu_hbi_ldst_rsp_itf, 1);
    BTI_IF_CONSTRUCT(s, va_i_, 1);
    BTI_IF_CONSTRUCT(s, va_d_, 1);
    BTI_IF_CONSTRUCT(s, pa_i_, 1);
    BTI_IF_CONSTRUCT(s, pa_d_, 1);
    BTI_IF_CONSTRUCT(s, pa_ptw_, 1);
    BTI_IF_CONSTRUCT(s, l1d_, 1);
    TLB_FLUSH_IF_CONSTRUCT(s, tlb_flush_itf, 1);
    L1_FLUSH_IF_CONSTRUCT(s, l1i_flush_itf, 1);
    HART_EXPT_IF_CONSTRUCT(s, mmu_fch_expt_itf, 1);
    HART_EXPT_IF_CONSTRUCT(s, fch_expt_itf, 1);
    HART_EXPT_IF_CONSTRUCT(s, ex_expt_itf, 1);
    HART_EXPT_IF_CONSTRUCT(s, ldst_expt_itf, 1);
    TRAP_SEND_IF_CONSTRUCT(s, trap_send_itf, 1);
    EXU_CSR_READ_REQ_SIGNAL_IF_CONSTRUCT(s, exu_csr_read_req_sig_itf, false, false);
    CSR_EXU_READ_RSP_SIGNAL_IF_CONSTRUCT(s, csr_exu_read_rsp_sig_itf, false, false);
    EXU_CSR_WRITE_REQ_SIGNAL_IF_CONSTRUCT(s, exu_csr_write_req_sig_itf, false, false);
    CSR_EXU_WRITE_RSP_SIGNAL_IF_CONSTRUCT(s, csr_exu_write_rsp_sig_itf, false, false);
    EXU_STATE_SIGNAL_IF_CONSTRUCT(s, exu_state_sig_itf, false, false);
    TRAP_EXU_CTRL_SIGNAL_IF_CONSTRUCT(s, trap_exu_ctrl_sig_itf, false, false);
    CSR_MMU_STATE_SIGNAL_IF_CONSTRUCT(s, csr_mmu_state_sig_itf, false, false);
    CSR_LSU_STATE_SIGNAL_IF_CONSTRUCT(s, csr_lsu_state_sig_itf, false, false);
    CSR_TRAP_STATE_SIGNAL_IF_CONSTRUCT(s, csr_trap_state_sig_itf, false, false);
    TRAP_CSR_WRITE_REQ_SIGNAL_IF_CONSTRUCT(s, trap_csr_write_req_sig_itf, false, false);
    CSR_TRAP_WRITE_RSP_SIGNAL_IF_CONSTRUCT(s, csr_trap_write_rsp_sig_itf, false, false);

    s->ifu.fch_req_mst = &s->fch_req_itf;
    s->ifu.fch_rsp_slv = &s->fch_rsp_itf;
    s->ifu.ex_req_mst = &s->ex_req_itf;
    s->ifu.ex_rsp_slv = &s->ex_rsp_itf;
    s->ifu.fl_req_mst = &s->fl_req_itf;
    s->ifu.fch_expt_mst = &s->fch_expt_itf;
    s->ifu.trap_send_slv = &s->trap_send_itf;
    s->ifu.tlb_flush_slv = &s->tlb_flush_itf;
    s->ifu.l1i_flush_slv = &s->l1i_flush_itf;
    s->ifu.exu_state_in = &s->exu_state_sig_itf;
    s->ifu.mod.cycle = s->mod.cycle;
    ifu_conf_t ifu_conf = {
        .reset_pc = conf->boot_rom_base,
        .boot_rom_base = conf->boot_rom_base,
        .boot_rom_size = conf->boot_rom_size,
        .ctrlq_depth = conf->ifu_ctrlq_depth,
        .fch_ost_depth = conf->ifu_fch_ost_depth
    };
    ifu_construct(&s->ifu, s->mod.hier_name, "u_ifu", &ifu_conf);

    s->exu.fl_req_slv = &s->fl_req_itf;
    s->exu.ex_req_slv = &s->ex_req_itf;
    s->exu.ex_rsp_mst = &s->ex_rsp_itf;
    s->exu.ldst_req_mst = &s->exu_lsu_ldst_req_itf;
    s->exu.ldst_rsp_slv = &s->exu_lsu_ldst_rsp_itf;
    s->exu.ex_expt_mst = &s->ex_expt_itf;
    s->exu.exu_csr_read_req_out = &s->exu_csr_read_req_sig_itf;
    s->exu.csr_exu_read_rsp_in = &s->csr_exu_read_rsp_sig_itf;
    s->exu.exu_csr_write_req_out = &s->exu_csr_write_req_sig_itf;
    s->exu.csr_exu_write_rsp_in = &s->csr_exu_write_rsp_sig_itf;
    s->exu.tlb_flush_mst = &s->tlb_flush_itf;
    s->exu.l1i_flush_mst = &s->l1i_flush_itf;
    s->exu.exu_state_out = &s->exu_state_sig_itf;
    s->exu.trap_exu_ctrl_in = &s->trap_exu_ctrl_sig_itf;
    s->exu.mod.cycle = s->mod.cycle;
    exu_construct(&s->exu, s->mod.hier_name, "u_exu");

    s->csr.mod.cycle = s->mod.cycle;
    s->csr.core_s_irq_slv = s->core_s_irq_slv;
    s->csr.core_timer_in = s->core_timer_in;
    s->csr.core_m_irq_in = s->core_m_irq_in;
    s->csr.core_swi_pend_out = s->core_swi_pend_out;
    s->csr.ext_irq_in = s->ext_irq_in;
    s->csr.exu_csr_read_req_in = &s->exu_csr_read_req_sig_itf;
    s->csr.csr_exu_read_rsp_out = &s->csr_exu_read_rsp_sig_itf;
    s->csr.exu_csr_write_req_in = &s->exu_csr_write_req_sig_itf;
    s->csr.csr_exu_write_rsp_out = &s->csr_exu_write_rsp_sig_itf;
    s->csr.trap_csr_write_req_in = &s->trap_csr_write_req_sig_itf;
    s->csr.csr_trap_write_rsp_out = &s->csr_trap_write_rsp_sig_itf;
    s->csr.csr_mmu_state_out = &s->csr_mmu_state_sig_itf;
    s->csr.csr_lsu_state_out = &s->csr_lsu_state_sig_itf;
    s->csr.csr_trap_state_out = &s->csr_trap_state_sig_itf;
    csr_construct(&s->csr, s->mod.hier_name, "u_csr");

    s->lsu.exu_ldst_req_slv = &s->exu_lsu_ldst_req_itf;
    s->lsu.exu_ldst_rsp_mst = &s->exu_lsu_ldst_rsp_itf;
    s->lsu.hbi_ldst_req_mst = &s->lsu_hbi_ldst_req_itf;
    s->lsu.hbi_ldst_rsp_slv = &s->lsu_hbi_ldst_rsp_itf;
    s->lsu.csr_lsu_state_in = &s->csr_lsu_state_sig_itf;
    s->lsu.mod.cycle = s->mod.cycle;
    lsu_conf_t lsu_conf = {
        .stg_fifo_depth = conf->lsu_stg_fifo_depth,
        .ost_depth = conf->lsu_ost_depth
    };
    lsu_construct(&s->lsu, s->mod.hier_name, "u_lsu", &lsu_conf);

    BTI_MST_CONNECT(&s->hbi, i_, s, va_i_);
    BTI_MST_CONNECT(&s->hbi, d_, s, va_d_);
    s->hbi.fch_req_slv = &s->fch_req_itf;
    s->hbi.fch_rsp_mst = &s->fch_rsp_itf;
    s->hbi.mmu_fch_expt_slv = &s->mmu_fch_expt_itf;
    s->hbi.ldst_req_slv = &s->lsu_hbi_ldst_req_itf;
    s->hbi.ldst_rsp_mst = &s->lsu_hbi_ldst_rsp_itf;
    s->hbi.mod.cycle = s->mod.cycle;
    hbi_conf_t hbi_conf = {
        .stg_fifo_depth = conf->hbi_stg_fifo_depth,
        .i_ost_depth = conf->hbi_i_ost_depth,
        .d_ost_depth = conf->hbi_d_ost_depth
    };
    hbi_construct(&s->hbi, s->mod.hier_name, "u_hbi", &hbi_conf);

    BTI_SLV_CONNECT(&s->mmu, va_i_, s, va_i_);
    BTI_SLV_CONNECT(&s->mmu, va_d_, s, va_d_);
    s->mmu.mmu_fch_expt_mst = &s->mmu_fch_expt_itf;
    s->mmu.ldst_expt_mst = &s->ldst_expt_itf;
    BTI_MST_CONNECT(&s->mmu, pa_i_, s, pa_i_);
    BTI_MST_CONNECT(&s->mmu, pa_d_, s, pa_d_);
    BTI_MST_CONNECT(&s->mmu, ptw_, s, pa_ptw_);
    s->mmu.tlb_flush_slv = &s->tlb_flush_itf;
    s->mmu.exu_state_in = &s->exu_state_sig_itf;
    s->mmu.csr_mmu_state_in = &s->csr_mmu_state_sig_itf;
    mmu_conf_t mmu_conf = {
        .i_stg_fifo_depth = conf->mmu_i_stg_fifo_depth,
        .d_stg_fifo_depth = conf->mmu_d_stg_fifo_depth,
        .ost_depth = conf->mmu_ost_depth
    };
    s->mmu.mod.cycle = s->mod.cycle;
    mmu_construct(&s->mmu, s->mod.hier_name, "u_mmu", &mmu_conf);

    BTI_SLV_ARR_CONNECT(&s->l1d_bti_mux, host_, 0, s, pa_d_);
    BTI_SLV_ARR_CONNECT(&s->l1d_bti_mux, host_, 1, s, pa_ptw_);
    BTI_MST_CONNECT(&s->l1d_bti_mux, gst_, s, l1d_);
    s->l1d_bti_mux.mod.cycle = s->mod.cycle;
    bti_mux_conf_t l1d_bti_mux_conf = {
        .host_num = 2,
        .stg_fifo_depth = conf->l1d_bti_mux_stg_fifo_depth,
        .ost_depth = conf->l1d_bti_mux_ost_depth
    };
    bti_mux_construct(&s->l1d_bti_mux, s->mod.hier_name, "u_l1d_bti_mux",
        &l1d_bti_mux_conf);

    l1_conf_t l1i_conf = {
        .full_bypass = false,
        .ro = true,
        .size = L1I_SIZE,
        .way_num = L1I_WAY_NUM,
        .latency = conf->l1_latency,
        .stg_fifo_depth = conf->l1i_stg_fifo_depth,
        .ost_depth = conf->l1_ost_depth,
        .bypass_bases = { conf->boot_rom_base, conf->itcm_base },
        .bypass_sizes = { conf->boot_rom_size, conf->itcm_size }
    };
    s->l1i.mod.cycle = s->mod.cycle;
    BTI_SLV_CONNECT(&s->l1i, , s, pa_i_);
    AXI4_MST_IMPORT(&s->l1i, , s, i_);
    s->l1i.flush_slv = &s->l1i_flush_itf;
    l1_construct(&s->l1i, s->mod.hier_name, "u_l1i", &l1i_conf);

    l1_conf_t l1d_conf = {
        .full_bypass = false,
        .ro = false,
        .size = L1D_SIZE,
        .way_num = L1D_WAY_NUM,
        .latency = conf->l1_latency,
        .stg_fifo_depth = conf->l1d_stg_fifo_depth,
        .ost_depth = conf->l1_ost_depth,
        .bypass_bases = { conf->boot_rom_base, conf->itcm_base, conf->dtcm_base, conf->cfg_base },
        .bypass_sizes = { conf->boot_rom_size, conf->itcm_size, conf->dtcm_size, conf->cfg_size }
    };
    s->l1d.mod.cycle = s->mod.cycle;
    BTI_SLV_CONNECT(&s->l1d, , s, l1d_);
    AXI4_MST_IMPORT(&s->l1d, , s, d_);
    s->l1d.flush_slv = NULL;
    l1_construct(&s->l1d, s->mod.hier_name, "u_l1d", &l1d_conf);

    s->trap.fch_expt_slv = &s->fch_expt_itf;
    s->trap.ex_expt_slv = &s->ex_expt_itf;
    s->trap.ldst_expt_slv = &s->ldst_expt_itf;
    s->trap.trap_send_mst = &s->trap_send_itf;
    s->trap.exu_state_in = &s->exu_state_sig_itf;
    s->trap.trap_exu_ctrl_out = &s->trap_exu_ctrl_sig_itf;
    s->trap.csr_trap_state_in = &s->csr_trap_state_sig_itf;
    s->trap.trap_csr_write_req_out = &s->trap_csr_write_req_sig_itf;
    s->trap.csr_trap_write_rsp_in = &s->csr_trap_write_rsp_sig_itf;
    s->trap.mod.cycle = s->mod.cycle;
    trap_construct(&s->trap, s->mod.hier_name, "u_trap");
}

void hart_reset(hart_t *s)
{
    mod_reset(&s->mod);
    itf_reset(&s->exu_state_sig_itf);
    itf_reset(&s->trap_exu_ctrl_sig_itf);
    itf_reset(&s->csr_mmu_state_sig_itf);
    itf_reset(&s->csr_lsu_state_sig_itf);
    itf_reset(&s->csr_trap_state_sig_itf);
    itf_reset(&s->trap_csr_write_req_sig_itf);
    itf_reset(&s->csr_trap_write_rsp_sig_itf);

    ifu_reset(&s->ifu);
    exu_reset(&s->exu);
    csr_reset(&s->csr);
    lsu_reset(&s->lsu);
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
    itf_reset(&s->exu_lsu_ldst_req_itf);
    itf_reset(&s->exu_lsu_ldst_rsp_itf);
    itf_reset(&s->lsu_hbi_ldst_req_itf);
    itf_reset(&s->lsu_hbi_ldst_rsp_itf);
    BTI_IF_RESET(s, va_i_);
    BTI_IF_RESET(s, va_d_);
    BTI_IF_RESET(s, pa_i_);
    BTI_IF_RESET(s, pa_d_);
    BTI_IF_RESET(s, pa_ptw_);
    BTI_IF_RESET(s, l1d_);
    itf_reset(&s->tlb_flush_itf);
    itf_reset(&s->l1i_flush_itf);
    itf_reset(&s->mmu_fch_expt_itf);
    itf_reset(&s->fch_expt_itf);
    itf_reset(&s->ex_expt_itf);
    itf_reset(&s->ldst_expt_itf);
    itf_reset(&s->trap_send_itf);
    itf_reset(&s->exu_csr_read_req_sig_itf);
    itf_reset(&s->csr_exu_read_rsp_sig_itf);
    itf_reset(&s->exu_csr_write_req_sig_itf);
    itf_reset(&s->csr_exu_write_rsp_sig_itf);
}

void hart_free(hart_t *s)
{
    mod_free(&s->mod);
    ifu_free(&s->ifu);
    exu_free(&s->exu);
    csr_free(&s->csr);
    lsu_free(&s->lsu);
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
    itf_free(&s->exu_lsu_ldst_req_itf);
    itf_free(&s->exu_lsu_ldst_rsp_itf);
    itf_free(&s->lsu_hbi_ldst_req_itf);
    itf_free(&s->lsu_hbi_ldst_rsp_itf);
    BTI_IF_FREE(s, va_i_);
    BTI_IF_FREE(s, va_d_);
    BTI_IF_FREE(s, pa_i_);
    BTI_IF_FREE(s, pa_d_);
    BTI_IF_FREE(s, pa_ptw_);
    BTI_IF_FREE(s, l1d_);
    itf_free(&s->tlb_flush_itf);
    itf_free(&s->l1i_flush_itf);
    itf_free(&s->mmu_fch_expt_itf);
    itf_free(&s->fch_expt_itf);
    itf_free(&s->ex_expt_itf);
    itf_free(&s->ldst_expt_itf);
    itf_free(&s->trap_send_itf);
    itf_free(&s->exu_csr_read_req_sig_itf);
    itf_free(&s->csr_exu_read_rsp_sig_itf);
    itf_free(&s->exu_csr_write_req_sig_itf);
    itf_free(&s->csr_exu_write_rsp_sig_itf);
    itf_free(&s->exu_state_sig_itf);
    itf_free(&s->trap_exu_ctrl_sig_itf);
    itf_free(&s->csr_mmu_state_sig_itf);
    itf_free(&s->csr_lsu_state_sig_itf);
    itf_free(&s->csr_trap_state_sig_itf);
    itf_free(&s->trap_csr_write_req_sig_itf);
    itf_free(&s->csr_trap_write_rsp_sig_itf);
}

void hart_clock(hart_t *s)
{
    mod_clock(&s->mod);
    exu_clock(&s->exu);
    csr_clock(&s->csr);
    lsu_clock(&s->lsu);
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
    itf_dbg_clock(&s->exu_lsu_ldst_req_itf);
    itf_dbg_clock(&s->exu_lsu_ldst_rsp_itf);
    itf_dbg_clock(&s->lsu_hbi_ldst_req_itf);
    itf_dbg_clock(&s->lsu_hbi_ldst_rsp_itf);
    BTI_IF_DBG_CLOCK(s, va_i_);
    BTI_IF_DBG_CLOCK(s, va_d_);
    BTI_IF_DBG_CLOCK(s, pa_i_);
    BTI_IF_DBG_CLOCK(s, pa_d_);
    BTI_IF_DBG_CLOCK(s, pa_ptw_);
    BTI_IF_DBG_CLOCK(s, l1d_);
    itf_dbg_clock(&s->tlb_flush_itf);
    itf_dbg_clock(&s->l1i_flush_itf);
    itf_dbg_clock(&s->mmu_fch_expt_itf);
    itf_dbg_clock(&s->fch_expt_itf);
    itf_dbg_clock(&s->ex_expt_itf);
    itf_dbg_clock(&s->ldst_expt_itf);
    itf_dbg_clock(&s->trap_send_itf);
    itf_dbg_clock(&s->exu_csr_read_req_sig_itf);
    itf_dbg_clock(&s->csr_exu_read_rsp_sig_itf);
    itf_dbg_clock(&s->exu_csr_write_req_sig_itf);
    itf_dbg_clock(&s->csr_exu_write_rsp_sig_itf);
    itf_dbg_clock(&s->exu_state_sig_itf);
    itf_dbg_clock(&s->trap_exu_ctrl_sig_itf);
    itf_dbg_clock(&s->csr_mmu_state_sig_itf);
    itf_dbg_clock(&s->csr_lsu_state_sig_itf);
    itf_dbg_clock(&s->csr_trap_state_sig_itf);
    itf_dbg_clock(&s->trap_csr_write_req_sig_itf);
    itf_dbg_clock(&s->csr_trap_write_rsp_sig_itf);
}
