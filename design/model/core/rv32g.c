#include "rv32g.h"
#include "dbg/vcd.h"

void rv32g_construct(rv32g_t *s, const char *parent, const char *name, const rv32g_conf_t *conf)
{
    mod_construct(&s->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    bool smp_opt = conf->smp_opt;

    BTI_IF_CONSTRUCT(s, boot_rom_, 1);
    BTI_IF_CONSTRUCT(s, itcm_i_, 1);
    BTI_IF_CONSTRUCT(s, itcm_d_, 1);
    BTI_IF_CONSTRUCT(s, dtcm_, 1);
    APB_IF_CONSTRUCT(s, aclint_cfg_, 1);
    APB_IF_CONSTRUCT(s, plic_cfg_, 1);
    AXI4_SIM_PROT_IF_CONSTRUCT(s, hart_i_, 1, smp_opt);
    AXI4_SIM_PROT_IF_CONSTRUCT(s, hart_d_, 1, smp_opt);
    AXI4_IF_CONSTRUCT(s, cbi_mm_i_, 2);
    AXI4_IF_CONSTRUCT(s, cbi_mm_d_, 2);
    CORE_TIMER_SIM_PROT_SIGNAL_IF_CONSTRUCT(s, core_timer_sig_itf, false, smp_opt);
    CORE_M_IRQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(s, core_m_irq_sig_itf, false, smp_opt);
    CORE_SWI_PEND_SIM_PROT_SIGNAL_IF_CONSTRUCT(s, core_swi_pend_sig_itf, false, smp_opt);
    CORE_S_IRQ_SIM_PROT_IF_CONSTRUCT(s, core_s_irq_itf, 1, smp_opt);
    EXT_IRQ_SIM_PROT_SIGNAL_IF_CONSTRUCT(s, conv_ext_irq_sig_itf, false, smp_opt);

    s->hart.mod.cycle = s->mod.cycle;
    AXI4_MST_CONNECT(&s->hart, i_, s, hart_i_);
    AXI4_MST_CONNECT(&s->hart, d_, s, hart_d_);
    s->hart.core_timer_in = &s->core_timer_sig_itf;
    s->hart.core_m_irq_in = &s->core_m_irq_sig_itf;
    s->hart.core_s_irq_slv = &s->core_s_irq_itf;
    s->hart.ext_irq_in = &s->conv_ext_irq_sig_itf;
    s->hart.core_swi_pend_out = &s->core_swi_pend_sig_itf;
    hart_conf_t hart_conf = {
        .boot_rom_base = conf->boot_rom_base,
        .boot_rom_size = conf->boot_rom_size,
        .itcm_base = conf->itcm_base,
        .itcm_size = conf->itcm_size,
        .dtcm_base = conf->dtcm_base,
        .dtcm_size = conf->dtcm_size,
        .cfg_base = conf->cfg_base,
        .cfg_size = conf->cfg_size,
        .ifu_ctrlq_depth = conf->hart_ifu_ctrlq_depth,
        .ifu_fch_ost_depth = conf->hart_ifu_fch_ost_depth,
        .hbi_stg_fifo_depth = conf->hart_hbi_stg_fifo_depth,
        .hbi_i_ost_depth = conf->hart_hbi_i_ost_depth,
        .hbi_d_ost_depth = conf->hart_hbi_d_ost_depth,
        .lsu_stg_fifo_depth = conf->hart_lsu_stg_fifo_depth,
        .lsu_ost_depth = conf->hart_lsu_ost_depth,
        .mmu_i_stg_fifo_depth = conf->hart_mmu_i_stg_fifo_depth,
        .mmu_d_stg_fifo_depth = conf->hart_mmu_d_stg_fifo_depth,
        .mmu_ost_depth = conf->hart_mmu_ost_depth,
        .l1_latency = conf->hart_l1_latency,
        .l1i_stg_fifo_depth = conf->hart_l1i_stg_fifo_depth,
        .l1d_stg_fifo_depth = conf->hart_l1d_stg_fifo_depth,
        .l1_ost_depth = conf->hart_l1_ost_depth,
        .l1d_bti_mux_stg_fifo_depth = conf->hart_l1d_bti_mux_stg_fifo_depth,
        .l1d_bti_mux_ost_depth = conf->hart_l1d_bti_mux_ost_depth
    };
    hart_construct(&s->hart, s->mod.hier_name, "u_hart", &hart_conf, smp_opt);

    s->cbi.mod.cycle = s->mod.cycle;
    AXI4_MST_CONNECT(&s->cbi, mm_i_, s, cbi_mm_i_);
    AXI4_MST_CONNECT(&s->cbi, mm_d_, s, cbi_mm_d_);
    APB_MST_IMPORT(&s->cbi, peri_, s, peri_);
    BTI_MST_CONNECT(&s->cbi, boot_rom_, s, boot_rom_);
    BTI_MST_CONNECT(&s->cbi, itcm_i_, s, itcm_i_);
    BTI_MST_CONNECT(&s->cbi, itcm_d_, s, itcm_d_);
    BTI_MST_CONNECT(&s->cbi, dtcm_, s, dtcm_);
    APB_MST_CONNECT(&s->cbi, aclint_cfg_, s, aclint_cfg_);
    APB_MST_CONNECT(&s->cbi, plic_cfg_, s, plic_cfg_);
    AXI4_SLV_CONNECT(&s->cbi, hart_i_, s, hart_i_);
    AXI4_SLV_CONNECT(&s->cbi, hart_d_, s, hart_d_);
    cbi_conf_t cbi_conf = {
        .boot_rom_base = conf->boot_rom_base,
        .boot_rom_size = conf->boot_rom_size,
        .itcm_base = conf->itcm_base,
        .itcm_size = conf->itcm_size,
        .dtcm_base = conf->dtcm_base,
        .dtcm_size = conf->dtcm_size,
        .mm_base = conf->mm_base,
        .mm_size = conf->mm_size,
        .cfg_base = conf->cfg_base,
        .cfg_size = conf->cfg_size,
        .peri_base = conf->peri_base,
        .peri_size = conf->peri_size,
        .aclint_base = conf->aclint_base,
        .aclint_size = conf->aclint_size,
        .plic_base = conf->plic_base,
        .plic_size = conf->plic_size,
        .bus_stg_fifo_depth = conf->cbi_bus_stg_fifo_depth,
        .bus_ost_depth = conf->cbi_bus_ost_depth
    };
    cbi_construct(&s->cbi, s->mod.hier_name, "u_cbi", &cbi_conf);

    s->l2.i_axi4_aw_slv = &s->cbi_mm_i_axi4_aw_itf;
    s->l2.i_axi4_w_slv = &s->cbi_mm_i_axi4_w_itf;
    s->l2.i_axi4_b_mst = &s->cbi_mm_i_axi4_b_itf;
    s->l2.i_axi4_ar_slv = &s->cbi_mm_i_axi4_ar_itf;
    s->l2.i_axi4_r_mst = &s->cbi_mm_i_axi4_r_itf;
    s->l2.d_axi4_aw_slv = &s->cbi_mm_d_axi4_aw_itf;
    s->l2.d_axi4_w_slv = &s->cbi_mm_d_axi4_w_itf;
    s->l2.d_axi4_b_mst = &s->cbi_mm_d_axi4_b_itf;
    s->l2.d_axi4_ar_slv = &s->cbi_mm_d_axi4_ar_itf;
    s->l2.d_axi4_r_mst = &s->cbi_mm_d_axi4_r_itf;
    s->l2.mem_axi4_aw_mst = s->mm_axi4_aw_mst;
    s->l2.mem_axi4_w_mst = s->mm_axi4_w_mst;
    s->l2.mem_axi4_b_slv = s->mm_axi4_b_slv;
    s->l2.mem_axi4_ar_mst = s->mm_axi4_ar_mst;
    s->l2.mem_axi4_r_slv = s->mm_axi4_r_slv;
    s->l2.mod.cycle = s->mod.cycle;
    l2_conf_t l2_conf = {
        .size = L2_SIZE,
        .way_num = L2_WAY_NUM,
        .latency = conf->l2_latency,
        .stg_fifo_depth = conf->l2_stg_fifo_depth,
        .bypass_ost_depth = conf->l2_bypass_ost_depth
    };
    l2_construct(&s->l2, s->mod.hier_name, "u_l2", &l2_conf);

    BTI_SLV_CONNECT(&s->boot_rom, , s, boot_rom_);
    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];
    s->boot_rom.mod.cycle = s->mod.cycle;
    rom_construct(&s->boot_rom, s->mod.hier_name, "u_boot_rom", ROM_MODE_BTI,
        conf->boot_rom_size, g_boot_code, g_boot_code_size, conf->boot_rom_base);

    BTI_SLV_ARR_CONNECT(&s->itcm, , 0, s, itcm_i_);
    BTI_SLV_ARR_CONNECT(&s->itcm, , 1, s, itcm_d_);
    s->itcm.mod.cycle = s->mod.cycle;
    ram_construct(&s->itcm, s->mod.hier_name, "u_itcm", 2, RAM_MODE_BTI,
        conf->itcm_size, conf->itcm_base, conf->itcm_latency);

    BTI_SLV_ARR_CONNECT(&s->dtcm, , 0, s, dtcm_);
    s->dtcm.mod.cycle = s->mod.cycle;
    ram_construct(&s->dtcm, s->mod.hier_name, "u_dtcm", 1, RAM_MODE_BTI,
        conf->dtcm_size, conf->dtcm_base, conf->dtcm_latency);

    s->aclint.mod.cycle = s->mod.cycle;
    APB_SLV_CONNECT(&s->aclint, cfg_, s, aclint_cfg_);
    s->aclint.core_timer_out = &s->core_timer_sig_itf;
    s->aclint.core_m_irq_outs[0] = &s->core_m_irq_sig_itf;
    s->aclint.core_s_irq_msts[0] = &s->core_s_irq_itf;
    s->aclint.core_swi_pend_ins[0] = &s->core_swi_pend_sig_itf;
    aclint_conf_t aclint_conf = {
        .mtimer_base = conf->aclint_mtimer_base,
        .mtimer_size = conf->aclint_mtimer_size,
        .mtimecmp_base = conf->aclint_mtimecmp_base,
        .mtimecmp_size = conf->aclint_mtimecmp_size,
        .mtimer_tick_cycles = conf->aclint_mtimer_tick_cycles,
        .mswi_base = conf->aclint_mswi_base,
        .mswi_size = conf->aclint_mswi_size,
        .sswi_base = conf->aclint_sswi_base,
        .sswi_size = conf->aclint_sswi_size
    };
    aclint_construct(&s->aclint, s->mod.hier_name, "u_aclint", &aclint_conf);

    APB_SLV_CONNECT(&s->plic, cfg_, s, plic_cfg_);
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        s->plic.div_ext_irq_ins[i] = s->ext_irq_ins[i];
    }
    s->plic.conv_ext_irq_out = &s->conv_ext_irq_sig_itf;
    plic_conf_t plic_conf = {};
    s->plic.mod.cycle = s->mod.cycle;
    plic_construct(&s->plic, s->mod.hier_name, "u_plic", &plic_conf);
}

void rv32g_reset(rv32g_t *s)
{
    mod_reset(&s->mod);
    hart_reset(&s->hart);
    cbi_reset(&s->cbi);
    rom_reset(&s->boot_rom);
    ram_reset(&s->itcm);
    ram_reset(&s->dtcm);
    aclint_reset(&s->aclint);
    plic_reset(&s->plic);
    l2_reset(&s->l2);

    BTI_IF_RESET(s, boot_rom_);
    BTI_IF_RESET(s, itcm_i_);
    BTI_IF_RESET(s, itcm_d_);
    BTI_IF_RESET(s, dtcm_);
    APB_IF_RESET(s, aclint_cfg_);
    APB_IF_RESET(s, plic_cfg_);
    AXI4_IF_RESET(s, hart_i_);
    AXI4_IF_RESET(s, hart_d_);
    AXI4_IF_RESET(s, cbi_mm_i_);
    AXI4_IF_RESET(s, cbi_mm_d_);
    itf_reset(&s->core_timer_sig_itf);
    itf_reset(&s->core_m_irq_sig_itf);
    itf_reset(&s->core_s_irq_itf);
    itf_reset(&s->core_swi_pend_sig_itf);
    itf_reset(&s->conv_ext_irq_sig_itf);
}

void rv32g_free(rv32g_t *s)
{
    mod_free(&s->mod);
    hart_free(&s->hart);
    cbi_free(&s->cbi);
    rom_free(&s->boot_rom);
    ram_free(&s->itcm);
    ram_free(&s->dtcm);
    aclint_free(&s->aclint);
    plic_free(&s->plic);
    l2_free(&s->l2);

    BTI_IF_FREE(s, boot_rom_);
    BTI_IF_FREE(s, itcm_i_);
    BTI_IF_FREE(s, itcm_d_);
    BTI_IF_FREE(s, dtcm_);
    APB_IF_FREE(s, aclint_cfg_);
    APB_IF_FREE(s, plic_cfg_);
    AXI4_IF_FREE(s, hart_i_);
    AXI4_IF_FREE(s, hart_d_);
    AXI4_IF_FREE(s, cbi_mm_i_);
    AXI4_IF_FREE(s, cbi_mm_d_);
    itf_free(&s->core_timer_sig_itf);
    itf_free(&s->core_m_irq_sig_itf);
    itf_free(&s->core_s_irq_itf);
    itf_free(&s->core_swi_pend_sig_itf);
    itf_free(&s->conv_ext_irq_sig_itf);
}

static void rv32g_rest_clock(rv32g_t *s)
{
    cbi_clock(&s->cbi);
    rom_clock(&s->boot_rom);
    ram_clock(&s->itcm);
    ram_clock(&s->dtcm);
    aclint_clock(&s->aclint);
    plic_clock(&s->plic);
    l2_clock(&s->l2);
}

void rv32g_clock(rv32g_t *s)
{
    mod_clock(&s->mod);
    hart_clock(&s->hart);
    rv32g_rest_clock(s);

    BTI_IF_DBG_CLOCK(s, boot_rom_);
    BTI_IF_DBG_CLOCK(s, itcm_i_);
    BTI_IF_DBG_CLOCK(s, itcm_d_);
    BTI_IF_DBG_CLOCK(s, dtcm_);
    APB_IF_DBG_CLOCK(s, aclint_cfg_);
    APB_IF_DBG_CLOCK(s, plic_cfg_);
    AXI4_IF_DBG_CLOCK(s, hart_i_);
    AXI4_IF_DBG_CLOCK(s, hart_d_);
    AXI4_IF_DBG_CLOCK(s, cbi_mm_i_);
    AXI4_IF_DBG_CLOCK(s, cbi_mm_d_);
    itf_dbg_clock(&s->core_timer_sig_itf);
    itf_dbg_clock(&s->core_m_irq_sig_itf);
    itf_dbg_clock(&s->core_s_irq_itf);
    itf_dbg_clock(&s->core_swi_pend_sig_itf);
    itf_dbg_clock(&s->conv_ext_irq_sig_itf);
}
