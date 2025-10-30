#include "hart.h"

void hart_construct(hart_t *s, const hart_conf_t *conf)
{
    itf_construct(&s->ex_req_itf, s->cycle, "ex_req_itf", &ex_req_if_to_str, sizeof(ex_req_if_t), 1);
    itf_construct(&s->ex_rsp_itf, s->cycle, "ex_rsp_itf", &ex_rsp_if_to_str, sizeof(ex_rsp_if_t), 1);
    itf_construct(&s->fl_req_itf, s->cycle, "fl_req_itf", &fl_req_if_to_str, sizeof(fl_req_if_t), 1);
    itf_construct(&s->fch_req_itf, s->cycle, "fch_req_itf", &fch_req_if_to_str, sizeof(fch_req_if_t), 1);
    itf_construct(&s->fch_rsp_itf, s->cycle, "fch_rsp_itf", &fch_rsp_if_to_str, sizeof(fch_rsp_if_t), 1);
    itf_construct(&s->ldst_req_itf, s->cycle, "ldst_req_itf", &ldst_req_if_to_str, sizeof(ldst_req_if_t), 1);
    itf_construct(&s->ldst_rsp_itf, s->cycle, "ldst_rsp_itf", &ldst_rsp_if_to_str, sizeof(ldst_rsp_if_t), 1);
    itf_construct(&s->va_i_bti_req_itf, s->cycle, "va_i_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->va_i_bti_rsp_itf, s->cycle, "va_i_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->va_d_bti_req_itf, s->cycle, "va_d_bti_req_itf", &bti_req_if_to_str, sizeof(bti_req_if_t), 1);
    itf_construct(&s->va_d_bti_rsp_itf, s->cycle, "va_d_bti_rsp_itf", &bti_rsp_if_to_str, sizeof(bti_rsp_if_t), 1);
    itf_construct(&s->hart_expt_itf, s->cycle, "hart_expt_itf", &hart_expt_if_to_str, sizeof(hart_expt_if_t), 1);
    itf_construct(&s->trap_send_itf, s->cycle, "trap_send_itf", &trap_send_if_to_str, sizeof(trap_send_if_t), 1);

    s->ifu.fch_req_mst = &s->fch_req_itf;
    s->ifu.fch_rsp_slv = &s->fch_rsp_itf;
    s->ifu.ex_req_mst = &s->ex_req_itf;
    s->ifu.ex_rsp_slv = &s->ex_rsp_itf;
    s->ifu.fl_req_mst = &s->fl_req_itf;
    s->ifu.trap_send_slv = &s->trap_send_itf;
    ifu_construct(&s->ifu, conf->boot_rom_base, conf->boot_rom_base, conf->boot_rom_size);

    s->exu.fl_req_slv = &s->fl_req_itf;
    s->exu.ex_req_slv = &s->ex_req_itf;
    s->exu.ex_rsp_mst = &s->ex_rsp_itf;
    s->exu.ldst_req_mst = &s->ldst_req_itf;
    s->exu.ldst_rsp_slv = &s->ldst_rsp_itf;
    s->exu.hart_expt_mst = &s->hart_expt_itf;
    exu_construct(&s->exu, &s->priv, &s->csr);

    s->hbi.i_bti_req_mst = &s->va_i_bti_req_itf;
    s->hbi.i_bti_rsp_slv = &s->va_i_bti_rsp_itf;
    s->hbi.d_bti_req_mst = &s->va_d_bti_req_itf;
    s->hbi.d_bti_rsp_slv = &s->va_d_bti_rsp_itf;
    s->hbi.fch_req_slv = &s->fch_req_itf;
    s->hbi.fch_rsp_mst = &s->fch_rsp_itf;
    s->hbi.ldst_req_slv = &s->ldst_req_itf;
    s->hbi.ldst_rsp_mst = &s->ldst_rsp_itf;
    hbi_construct(&s->hbi);

    s->mmu.va_i_bti_req_slv = &s->va_i_bti_req_itf;
    s->mmu.va_i_bti_rsp_mst = &s->va_i_bti_rsp_itf;
    s->mmu.va_d_bti_req_slv = &s->va_d_bti_req_itf;
    s->mmu.va_d_bti_rsp_mst = &s->va_d_bti_rsp_itf;
    s->mmu.pa_i_bti_req_mst = s->i_bti_req_mst;
    s->mmu.pa_i_bti_rsp_slv = s->i_bti_rsp_slv;
    s->mmu.pa_d_bti_req_mst = s->d_bti_req_mst;
    s->mmu.pa_d_bti_rsp_slv = s->d_bti_rsp_slv;
    mmu_conf_t mmu_conf = {};
    mmu_construct(&s->mmu, &mmu_conf);

    s->trap.trap_send_mst = &s->trap_send_itf;
    s->trap.hart_expt_slv = &s->hart_expt_itf;
    s->trap.core_irq_slv = s->core_irq_slv;
    s->trap.ext_irq_slv = s->ext_irq_slv;
    trap_construct(&s->trap);
}

void hart_reset(hart_t *s)
{
    s->priv = RV32G_PRIV_MACHINE;

    ifu_reset(&s->ifu);
    exu_reset(&s->exu);
    csr_reset(&s->csr);
    hbi_reset(&s->hbi);
    mmu_reset(&s->mmu);
    trap_reset(&s->trap);
}

void hart_free(hart_t *s)
{
    ifu_free(&s->ifu);
    exu_free(&s->exu);
    hbi_free(&s->hbi);
    mmu_free(&s->mmu);
    trap_free(&s->trap);

    itf_free(&s->ex_req_itf);
    itf_free(&s->ex_rsp_itf);
    itf_free(&s->fl_req_itf);
    itf_free(&s->fch_req_itf);
    itf_free(&s->fch_rsp_itf);
    itf_free(&s->ldst_req_itf);
    itf_free(&s->ldst_rsp_itf);
    itf_free(&s->va_i_bti_req_itf);
    itf_free(&s->va_i_bti_rsp_itf);
    itf_free(&s->va_d_bti_req_itf);
    itf_free(&s->va_d_bti_rsp_itf);
    itf_free(&s->hart_expt_itf);
    itf_free(&s->trap_send_itf);
}

void hart_clock(hart_t *s)
{
    s->csr.cycle = (u32)(*s->cycle);
    s->csr.cycleh = (u32)(*s->cycle >> 32u);

    exu_clock(&s->exu);
    ifu_clock(&s->ifu);
    hbi_clock(&s->hbi);
    mmu_clock(&s->mmu);
    trap_clock(&s->trap);
}