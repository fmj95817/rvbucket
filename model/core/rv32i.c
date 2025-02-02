#include "rv32i.h"
#include "dbg.h"

void rv32i_construct(rv32i_t *s, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size)
{
    itf_construct(&s->fch_req_itf, sizeof(fch_req_if_t), 1);
    itf_construct(&s->fch_rsp_itf, sizeof(fch_rsp_if_t), 1);
    itf_construct(&s->ex_req_itf, sizeof(ex_req_if_t), 1);
    itf_construct(&s->ex_rsp_itf, sizeof(ex_rsp_if_t), 1);
    itf_construct(&s->fl_req_itf, sizeof(fl_req_if_t), 1);
    itf_construct(&s->fl_rsp_itf, sizeof(fl_rsp_if_t), 1);
    itf_construct(&s->ldst_req_itf, sizeof(ldst_req_if_t), 1);
    itf_construct(&s->ldst_rsp_itf, sizeof(fch_rsp_if_t), 1);

    s->ifu.fch_req_mst = &s->fch_req_itf;
    s->ifu.fch_rsp_slv = &s->fch_rsp_itf;
    s->ifu.ex_req_mst = &s->ex_req_itf;
    s->ifu.ex_rsp_slv = &s->ex_rsp_itf;
    s->ifu.fl_req_mst = &s->fl_req_itf;
    s->ifu.fl_rsp_slv = &s->fl_rsp_itf;

    s->exu.fl_req_slv = &s->fl_req_itf;
    s->exu.fl_rsp_mst = &s->fl_rsp_itf;
    s->exu.ex_req_slv = &s->ex_req_itf;
    s->exu.ex_rsp_mst = &s->ex_rsp_itf;
    s->exu.ldst_req_mst = &s->ldst_req_itf;
    s->exu.ldst_rsp_slv = &s->ldst_rsp_itf;

    s->biu.bti_req_mst = s->bti_req_mst;
    s->biu.bti_rsp_slv = s->bti_rsp_slv;
    s->biu.fch_req_slv = &s->fch_req_itf;
    s->biu.fch_rsp_mst = &s->fch_rsp_itf;
    s->biu.ldst_req_slv = &s->ldst_req_itf;
    s->biu.ldst_rsp_mst = &s->ldst_rsp_itf;

    ifu_construct(&s->ifu, reset_pc, boot_rom_base, boot_rom_size);
    exu_construct(&s->exu);
    biu_construct(&s->biu);
}

void rv32i_reset(rv32i_t *s)
{
    ifu_reset(&s->ifu);
    exu_reset(&s->exu);
    biu_reset(&s->biu);
}

void rv32i_free(rv32i_t *s)
{
    ifu_free(&s->ifu);
    exu_free(&s->exu);
    biu_free(&s->biu);

    itf_free(&s->fch_req_itf);
    itf_free(&s->fch_rsp_itf);
    itf_free(&s->ex_req_itf);
    itf_free(&s->ex_rsp_itf);
    itf_free(&s->fl_req_itf);
    itf_free(&s->fl_rsp_itf);
    itf_free(&s->ldst_req_itf);
    itf_free(&s->ldst_rsp_itf);
}

void rv32i_clock(rv32i_t *s)
{
    ifu_clock(&s->ifu);
    exu_clock(&s->exu);
    biu_clock(&s->biu);
}