#include "rv32g.h"
#include "dbg/vcd.h"

void rv32g_construct(rv32g_t *s, const char *name, const rv32g_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    BTI_IF_CONSTRUCT(s, boot_rom_, 1);
    BTI_IF_CONSTRUCT(s, itcm_i_, 1);
    BTI_IF_CONSTRUCT(s, itcm_d_, 1);
    BTI_IF_CONSTRUCT(s, dtcm_, 1);
    APB_IF_CONSTRUCT(s, aclint_cfg_, 1);
    APB_IF_CONSTRUCT(s, plic_cfg_, 1);
    AXI4_IF_CONSTRUCT(s, hart_i_, 1);
    AXI4_IF_CONSTRUCT(s, hart_d_, 1);
    CORE_TIMER_SIGNAL_IF_CONSTRUCT(s, core_timer_sig_itf, false, false);
    CORE_M_IRQ_SIGNAL_IF_CONSTRUCT(s, core_m_irq_sig_itf, false, false);
    CORE_SWI_PEND_SIGNAL_IF_CONSTRUCT(s, core_swi_pend_sig_itf, false, false);
    CORE_S_IRQ_IF_CONSTRUCT(s, core_s_irq_itf, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(s, conv_ext_irq_sig_itf, false, false);

    s->hart.cycle = s->cycle;
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
        .cfg_size = conf->cfg_size
    };
    hart_construct(&s->hart, "u_hart", &hart_conf);

    s->cbi.cycle = s->cycle;
    AXI4_MST_IMPORT(&s->cbi, mm_i_, s, mm_i_);
    AXI4_MST_IMPORT(&s->cbi, mm_d_, s, mm_d_);
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
        .plic_size = conf->plic_size
    };
    cbi_construct(&s->cbi, "u_cbi", &cbi_conf);

    BTI_SLV_CONNECT(&s->boot_rom, , s, boot_rom_);
    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];
    rom_construct(&s->boot_rom, "u_boot_rom", ROM_MODE_BTI, conf->boot_rom_size, g_boot_code, g_boot_code_size, conf->boot_rom_base);

    BTI_SLV_ARR_CONNECT(&s->itcm, , 0, s, itcm_i_);
    BTI_SLV_ARR_CONNECT(&s->itcm, , 1, s, itcm_d_);
    ram_construct(&s->itcm, "u_itcm", 2, RAM_MODE_BTI, conf->itcm_size, conf->itcm_base);

    BTI_SLV_ARR_CONNECT(&s->dtcm, , 0, s, dtcm_);
    ram_construct(&s->dtcm, "u_dtcm", 1, RAM_MODE_BTI, conf->dtcm_size, conf->dtcm_base);

    s->aclint.cycle= s->cycle;
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
    aclint_construct(&s->aclint, "u_aclint", &aclint_conf);

    APB_SLV_CONNECT(&s->plic, cfg_, s, plic_cfg_);
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        s->plic.div_ext_irq_ins[i] = s->ext_irq_ins[i];
    }
    s->plic.conv_ext_irq_out = &s->conv_ext_irq_sig_itf;
    plic_conf_t plic_conf = {};
    plic_construct(&s->plic, "u_plic", &plic_conf);
}

void rv32g_reset(rv32g_t *s)
{
    hart_reset(&s->hart);
    cbi_reset(&s->cbi);
    rom_reset(&s->boot_rom);
    ram_reset(&s->itcm);
    ram_reset(&s->dtcm);
    aclint_reset(&s->aclint);
    plic_reset(&s->plic);

    BTI_IF_RESET(s, boot_rom_);
    BTI_IF_RESET(s, itcm_i_);
    BTI_IF_RESET(s, itcm_d_);
    BTI_IF_RESET(s, dtcm_);
    APB_IF_RESET(s, aclint_cfg_);
    APB_IF_RESET(s, plic_cfg_);
    AXI4_IF_RESET(s, hart_i_);
    AXI4_IF_RESET(s, hart_d_);
    itf_reset(&s->core_timer_sig_itf);
    itf_reset(&s->core_m_irq_sig_itf);
    itf_reset(&s->core_s_irq_itf);
    itf_reset(&s->core_swi_pend_sig_itf);
    itf_reset(&s->conv_ext_irq_sig_itf);
}

void rv32g_free(rv32g_t *s)
{
    hart_free(&s->hart);
    cbi_free(&s->cbi);
    rom_free(&s->boot_rom);
    ram_free(&s->itcm);
    ram_free(&s->dtcm);
    aclint_free(&s->aclint);
    plic_free(&s->plic);

    BTI_IF_FREE(s, boot_rom_);
    BTI_IF_FREE(s, itcm_i_);
    BTI_IF_FREE(s, itcm_d_);
    BTI_IF_FREE(s, dtcm_);
    APB_IF_FREE(s, aclint_cfg_);
    APB_IF_FREE(s, plic_cfg_);
    AXI4_IF_FREE(s, hart_i_);
    AXI4_IF_FREE(s, hart_d_);
    itf_free(&s->core_timer_sig_itf);
    itf_free(&s->core_m_irq_sig_itf);
    itf_free(&s->core_s_irq_itf);
    itf_free(&s->core_swi_pend_sig_itf);
    itf_free(&s->conv_ext_irq_sig_itf);
}

void rv32g_clock(rv32g_t *s)
{
    hart_clock(&s->hart);
    cbi_clock(&s->cbi);
    rom_clock(&s->boot_rom);
    ram_clock(&s->itcm);
    ram_clock(&s->dtcm);
    aclint_clock(&s->aclint);
    plic_clock(&s->plic);

    BTI_IF_DBG_CLOCK(s, boot_rom_);
    BTI_IF_DBG_CLOCK(s, itcm_i_);
    BTI_IF_DBG_CLOCK(s, itcm_d_);
    BTI_IF_DBG_CLOCK(s, dtcm_);
    APB_IF_DBG_CLOCK(s, aclint_cfg_);
    APB_IF_DBG_CLOCK(s, plic_cfg_);
    AXI4_IF_DBG_CLOCK(s, hart_i_);
    AXI4_IF_DBG_CLOCK(s, hart_d_);
    itf_dbg_clock(&s->core_timer_sig_itf);
    itf_dbg_clock(&s->core_m_irq_sig_itf);
    itf_dbg_clock(&s->core_s_irq_itf);
    itf_dbg_clock(&s->core_swi_pend_sig_itf);
    itf_dbg_clock(&s->conv_ext_irq_sig_itf);
}
