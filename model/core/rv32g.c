#include "rv32g.h"
#include "dbg/vcd.h"

void rv32g_construct(rv32g_t *s, const char *name, const rv32g_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    BTI_REQ_IF_CONSTRUCT(s, boot_rom_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(s, boot_rom_bti_rsp_itf, 1);
    BTI_REQ_IF_CONSTRUCT(s, itcm_i_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(s, itcm_i_bti_rsp_itf, 1);
    BTI_REQ_IF_CONSTRUCT(s, itcm_d_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(s, itcm_d_bti_rsp_itf, 1);
    BTI_REQ_IF_CONSTRUCT(s, dtcm_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(s, dtcm_bti_rsp_itf, 1);
    APB_REQ_IF_CONSTRUCT(s, aclint_cfg_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(s, aclint_cfg_apb_rsp_itf, 1);
    APB_REQ_IF_CONSTRUCT(s, plic_cfg_apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(s, plic_cfg_apb_rsp_itf, 1);
    BTI_REQ_IF_CONSTRUCT(s, hart_i_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(s, hart_i_bti_rsp_itf, 1);
    BTI_REQ_IF_CONSTRUCT(s, hart_d_bti_req_itf, 1);
    BTI_RSP_IF_CONSTRUCT(s, hart_d_bti_rsp_itf, 1);
    CORE_TIMER_SIGNAL_IF_CONSTRUCT(s, core_timer_sig_itf, false, false);
    CORE_M_IRQ_SIGNAL_IF_CONSTRUCT(s, core_m_irq_sig_itf, false, false);
    CORE_SWI_PEND_SIGNAL_IF_CONSTRUCT(s, core_swi_pend_sig_itf, false, false);
    CORE_S_IRQ_IF_CONSTRUCT(s, core_s_irq_itf, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(s, conv_ext_irq_sig_itf, false, false);

    s->hart.cycle = s->cycle;
    s->hart.i_bti_req_mst = &s->hart_i_bti_req_itf;
    s->hart.i_bti_rsp_slv = &s->hart_i_bti_rsp_itf;
    s->hart.d_bti_req_mst = &s->hart_d_bti_req_itf;
    s->hart.d_bti_rsp_slv = &s->hart_d_bti_rsp_itf;
    s->hart.core_timer_in = &s->core_timer_sig_itf;
    s->hart.core_m_irq_in = &s->core_m_irq_sig_itf;
    s->hart.core_s_irq_slv = &s->core_s_irq_itf;
    s->hart.ext_irq_in = &s->conv_ext_irq_sig_itf;
    s->hart.core_swi_pend_out = &s->core_swi_pend_sig_itf;
    hart_conf_t hart_conf = {
        .boot_rom_base = conf->boot_rom_base,
        .boot_rom_size = conf->boot_rom_size
    };
    hart_construct(&s->hart, "u_hart", &hart_conf);

    s->cbi.cycle = s->cycle;
    s->cbi.mm_i_bti_req_mst = s->mm_i_bti_req_mst;
    s->cbi.mm_i_bti_rsp_slv = s->mm_i_bti_rsp_slv;
    s->cbi.mm_d_bti_req_mst = s->mm_d_bti_req_mst;
    s->cbi.mm_d_bti_rsp_slv = s->mm_d_bti_rsp_slv;
    s->cbi.peri_apb_req_mst = s->peri_apb_req_mst;
    s->cbi.peri_apb_rsp_slv = s->peri_apb_rsp_slv;
    s->cbi.boot_rom_bti_req_mst = &s->boot_rom_bti_req_itf;
    s->cbi.boot_rom_bti_rsp_slv = &s->boot_rom_bti_rsp_itf;
    s->cbi.itcm_i_bti_req_mst = &s->itcm_i_bti_req_itf;
    s->cbi.itcm_i_bti_rsp_slv = &s->itcm_i_bti_rsp_itf;
    s->cbi.itcm_d_bti_req_mst = &s->itcm_d_bti_req_itf;
    s->cbi.itcm_d_bti_rsp_slv = &s->itcm_d_bti_rsp_itf;
    s->cbi.dtcm_bti_req_mst = &s->dtcm_bti_req_itf;
    s->cbi.dtcm_bti_rsp_slv = &s->dtcm_bti_rsp_itf;
    s->cbi.aclint_cfg_apb_req_mst = &s->aclint_cfg_apb_req_itf;
    s->cbi.aclint_cfg_apb_rsp_slv = &s->aclint_cfg_apb_rsp_itf;
    s->cbi.plic_cfg_apb_req_mst = &s->plic_cfg_apb_req_itf;
    s->cbi.plic_cfg_apb_rsp_slv = &s->plic_cfg_apb_rsp_itf;
    s->cbi.hart_i_bti_req_slv = &s->hart_i_bti_req_itf;
    s->cbi.hart_i_bti_rsp_mst = &s->hart_i_bti_rsp_itf;
    s->cbi.hart_d_bti_req_slv = &s->hart_d_bti_req_itf;
    s->cbi.hart_d_bti_rsp_mst = &s->hart_d_bti_rsp_itf;
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

    s->boot_rom.bti_req_slv = &s->boot_rom_bti_req_itf;
    s->boot_rom.bti_rsp_mst = &s->boot_rom_bti_rsp_itf;
    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];
    rom_construct(&s->boot_rom, "u_boot_rom", conf->boot_rom_size, g_boot_code, g_boot_code_size, conf->boot_rom_base);

    s->itcm.bti_req_slv[0] = &s->itcm_i_bti_req_itf;
    s->itcm.bti_rsp_mst[0] = &s->itcm_i_bti_rsp_itf;
    s->itcm.bti_req_slv[1] = &s->itcm_d_bti_req_itf;
    s->itcm.bti_rsp_mst[1] = &s->itcm_d_bti_rsp_itf;
    ram_construct(&s->itcm, "u_itcm", 2, conf->itcm_size, conf->itcm_base);

    s->dtcm.bti_req_slv[0] = &s->dtcm_bti_req_itf;
    s->dtcm.bti_rsp_mst[0] = &s->dtcm_bti_rsp_itf;
    ram_construct(&s->dtcm, "u_dtcm", 1, conf->dtcm_size, conf->dtcm_base);

    s->aclint.cycle= s->cycle;
    s->aclint.cfg_apb_req_slv = &s->aclint_cfg_apb_req_itf;
    s->aclint.cfg_apb_rsp_mst = &s->aclint_cfg_apb_rsp_itf;
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

    s->plic.cfg_apb_req_slv = &s->plic_cfg_apb_req_itf;
    s->plic.cfg_apb_rsp_mst = &s->plic_cfg_apb_rsp_itf;
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

    itf_free(&s->boot_rom_bti_req_itf);
    itf_free(&s->boot_rom_bti_rsp_itf);
    itf_free(&s->itcm_i_bti_req_itf);
    itf_free(&s->itcm_i_bti_rsp_itf);
    itf_free(&s->itcm_d_bti_req_itf);
    itf_free(&s->itcm_d_bti_rsp_itf);
    itf_free(&s->dtcm_bti_req_itf);
    itf_free(&s->dtcm_bti_rsp_itf);
    itf_free(&s->aclint_cfg_apb_req_itf);
    itf_free(&s->aclint_cfg_apb_rsp_itf);
    itf_free(&s->plic_cfg_apb_req_itf);
    itf_free(&s->plic_cfg_apb_rsp_itf);
    itf_free(&s->hart_i_bti_req_itf);
    itf_free(&s->hart_i_bti_rsp_itf);
    itf_free(&s->hart_d_bti_req_itf);
    itf_free(&s->hart_d_bti_rsp_itf);
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

    itf_dbg_clock(&s->boot_rom_bti_req_itf);
    itf_dbg_clock(&s->boot_rom_bti_rsp_itf);
    itf_dbg_clock(&s->itcm_i_bti_req_itf);
    itf_dbg_clock(&s->itcm_i_bti_rsp_itf);
    itf_dbg_clock(&s->itcm_d_bti_req_itf);
    itf_dbg_clock(&s->itcm_d_bti_rsp_itf);
    itf_dbg_clock(&s->dtcm_bti_req_itf);
    itf_dbg_clock(&s->dtcm_bti_rsp_itf);
    itf_dbg_clock(&s->aclint_cfg_apb_req_itf);
    itf_dbg_clock(&s->aclint_cfg_apb_rsp_itf);
    itf_dbg_clock(&s->plic_cfg_apb_req_itf);
    itf_dbg_clock(&s->plic_cfg_apb_rsp_itf);
    itf_dbg_clock(&s->hart_i_bti_req_itf);
    itf_dbg_clock(&s->hart_i_bti_rsp_itf);
    itf_dbg_clock(&s->hart_d_bti_req_itf);
    itf_dbg_clock(&s->hart_d_bti_rsp_itf);
    itf_dbg_clock(&s->core_timer_sig_itf);
    itf_dbg_clock(&s->core_m_irq_sig_itf);
    itf_dbg_clock(&s->core_s_irq_itf);
    itf_dbg_clock(&s->core_swi_pend_sig_itf);
    itf_dbg_clock(&s->conv_ext_irq_sig_itf);
}
