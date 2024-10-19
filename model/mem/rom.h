#ifndef ROM_H
#define ROM_H

#include "types.h"
#include "bus.h"

typedef struct rom {
    u32 size;
    u8 *data;
} rom_t;

extern void rom_construct(rom_t *rom, u32 size, const void *data, u32 data_size);
extern void rom_reset(rom_t *rom);
extern void rom_free(rom_t *rom);

extern void rom_burn(rom_t *rom, const void *data, u32 addr, u32 size);
extern bool rom_read(rom_t *rom, u32 addr, u32 *data);
extern bus_rsp_t rom_bus_req_handler(rom_t *rom, u32 base_addr, const bus_req_t *req);

#endif