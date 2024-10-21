#include "rv32i.h"
#include "isa.h"
#include "exu.h"
#include "dbg.h"

void rv32i_construct(rv32i_t *s, bus_if_t *bus_if, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size)
{
    ifu_construct(&s->ifu, &s->biu, reset_pc, boot_rom_base, boot_rom_size);
    exu_construct(&s->exu, &s->lsu);
    lsu_construct(&s->lsu, &s->biu);
    biu_construct(&s->biu, bus_if);
}

void rv32i_reset(rv32i_t *s)
{
    ifu_reset(&s->ifu);
    exu_reset(&s->exu);
    lsu_reset(&s->lsu);
    biu_reset(&s->biu);
}

void rv32i_free(rv32i_t *s)
{
    ifu_free(&s->ifu);
    exu_free(&s->exu);
    lsu_free(&s->lsu);
    biu_free(&s->biu);
}

void rv32i_exec(rv32i_t *s)
{
    ifu2exu_trans_t ifu2exu;
    ifu_exec(&s->ifu, &ifu2exu);
    exu_exec(&s->exu, &ifu2exu);
    ifu_update(&s->ifu, &ifu2exu);
}