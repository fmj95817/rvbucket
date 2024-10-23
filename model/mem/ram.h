#ifndef SRAM_H
#define SRAM_H

#include "types.h"
#include "bus.h"

typedef struct ram {
    u32 size;
    u8 *data;
} ram_t;

extern void ram_construct(ram_t *ram, u32 size);
extern void ram_reset(ram_t *ram);
extern void ram_free(ram_t *ram);

extern void ram_bus_trans_handler(ram_t *ram, u32 base_addr, bus_trans_if_t *i);

#endif