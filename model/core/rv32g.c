#include "rv32g.h"

void rv32g_construct(rv32g_t *s, const rv32g_conf_t *conf)
{
    itf_construct(&s->boot_rom_bti_req_itf, s->cycle, "boot_rom_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->boot_rom_bti_rsp_itf, s->cycle, "boot_rom_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->itcm_i_bti_req_itf, s->cycle, "itcm_i_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->itcm_i_bti_rsp_itf, s->cycle, "itcm_i_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->itcm_d_bti_req_itf, s->cycle, "itcm_d_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->itcm_d_bti_rsp_itf, s->cycle, "itcm_d_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->dtcm_bti_req_itf, s->cycle, "dtcm_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->dtcm_bti_rsp_itf, s->cycle, "dtcm_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->aclint_cfg_apb_req_itf, s->cycle, "aclint_cfg_apb_req_itf", &apb_req_if_to_str, sizeof(apb_req_if_t), 1);
    itf_construct(&s->aclint_cfg_apb_rsp_itf, s->cycle, "aclint_cfg_apb_rsp_itf", &apb_rsp_if_to_str, sizeof(apb_rsp_if_t), 1);
    itf_construct(&s->plic_cfg_apb_req_itf, s->cycle, "plic_cfg_apb_req_itf", &apb_req_if_to_str, sizeof(apb_req_if_t), 1);
    itf_construct(&s->plic_cfg_apb_rsp_itf, s->cycle, "plic_cfg_apb_rsp_itf", &apb_rsp_if_to_str, sizeof(apb_rsp_if_t), 1);
    itf_construct(&s->hart_i_bti_req_itf, s->cycle, "hart_i_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->hart_i_bti_rsp_itf, s->cycle, "hart_i_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->hart_d_bti_req_itf, s->cycle, "hart_d_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->hart_d_bti_rsp_itf, s->cycle, "hart_d_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->core_irq_itf, s->cycle, "core_irq_itf", &core_irq_if_to_str, sizeof(core_irq_if_t), 1);
    itf_construct(&s->conv_ext_irq_itf, s->cycle, "conv_ext_irq_itf", &ext_irq_if_to_str, sizeof(ext_irq_if_t), 1);

    s->hart.cycle = s->cycle;
    s->hart.i_bti_req_mst = &s->hart_i_bti_req_itf;
    s->hart.i_bti_rsp_slv = &s->hart_i_bti_rsp_itf;
    s->hart.d_bti_req_mst = &s->hart_d_bti_req_itf;
    s->hart.d_bti_rsp_slv = &s->hart_d_bti_rsp_itf;
    s->hart.core_irq_slv = &s->core_irq_itf;
    s->hart.ext_irq_slv = &s->conv_ext_irq_itf;
    hart_conf_t hart_conf = {
        .boot_rom_base = conf->boot_rom_base,
        .boot_rom_size = conf->boot_rom_size
    };
    hart_construct(&s->hart, &hart_conf);

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
    cbi_construct(&s->cbi, &cbi_conf);

    s->boot_rom.bti_req_slv = &s->boot_rom_bti_req_itf;
    s->boot_rom.bti_rsp_mst = &s->boot_rom_bti_rsp_itf;
    extern u32 g_boot_code_size;
    extern u8 g_boot_code[];
    rom_construct(&s->boot_rom, conf->boot_rom_size, g_boot_code, g_boot_code_size, conf->boot_rom_base);

    s->itcm.bti_req_slv[0] = &s->itcm_i_bti_req_itf;
    s->itcm.bti_rsp_mst[0] = &s->itcm_i_bti_rsp_itf;
    s->itcm.bti_req_slv[1] = &s->itcm_d_bti_req_itf;
    s->itcm.bti_rsp_mst[1] = &s->itcm_d_bti_rsp_itf;
    ram_construct(&s->itcm, 2, conf->itcm_size, conf->itcm_base);

    s->dtcm.bti_req_slv[0] = &s->dtcm_bti_req_itf;
    s->dtcm.bti_rsp_mst[0] = &s->dtcm_bti_rsp_itf;
    ram_construct(&s->dtcm, 1, conf->dtcm_size, conf->dtcm_base);

    s->aclint.cycle= s->cycle;
    s->aclint.csr_stimecmp = &s->hart.csr.stimecmp;
    s->aclint.csr_stimecmph = &s->hart.csr.stimecmph;
    s->aclint.cfg_apb_req_slv = &s->aclint_cfg_apb_req_itf;
    s->aclint.cfg_apb_rsp_mst = &s->aclint_cfg_apb_rsp_itf;
    s->aclint.core_irq_mst = &s->core_irq_itf;
    aclint_conf_t aclint_conf = {
        .mtimer_base = conf->aclint_mtimer_base,
        .mtimer_size = conf->aclint_mtimer_size,
        .mtimecmp_base = conf->aclint_mtimecmp_base,
        .mtimecmp_size = conf->aclint_mtimecmp_size,
        .mswi_base = conf->aclint_mswi_base,
        .mswi_size = conf->aclint_mswi_size,
        .sswi_base = conf->aclint_sswi_base,
        .sswi_size = conf->aclint_sswi_size
    };
    aclint_construct(&s->aclint, &aclint_conf);

    s->plic.cfg_apb_req_slv = &s->plic_cfg_apb_req_itf;
    s->plic.cfg_apb_rsp_mst = &s->plic_cfg_apb_rsp_itf;
    for (u32 i = 0; i < PLIC_MAX_IRQ_NUM; i++) {
        s->plic.div_ext_irq_slvs[i] = s->ext_irq_slvs[i];
    }
    s->plic.conv_ext_irq_mst = &s->conv_ext_irq_itf;
    plic_conf_t plic_conf = {};
    plic_construct(&s->plic, &plic_conf);
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
    itf_free(&s->core_irq_itf);
    itf_free(&s->conv_ext_irq_itf);
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
}