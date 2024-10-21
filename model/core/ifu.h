#ifndef IFU_H
#define IFU_H

#include "types.h"
#include "lsu.h"

typedef struct ifu {
    biu_t *biu;
    u32 pc;
    u32 reset_pc;

    struct {
        u32 base;
        u32 size;
    } boot_rom_info;
} ifu_t;

extern void ifu_construct(ifu_t *ifu, biu_t *biu, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size);
extern void ifu_reset(ifu_t *ifu);
extern void ifu_free(ifu_t *ifu);

extern void ifu_exec(ifu_t *ifu, iexec_if_t *i);
extern void ifu_update(ifu_t *ifu, const iexec_if_t *i);

#endif