#ifndef ROM_H
#define ROM_H

#include "base/types.h"
#include "base/itf.h"
#include "bti.h"

typedef struct rom {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;

    u32 base_addr;
    u32 size;
    u8 *data;
} rom_t;

extern void rom_construct(rom_t *rom, u32 size, const void *data, u32 data_size, u32 base_addr);
extern void rom_reset(rom_t *rom);
extern void rom_clock(rom_t *rom);
extern void rom_free(rom_t *rom);

extern void rom_burn(rom_t *rom, const void *data, u32 addr, u32 size);

#endif