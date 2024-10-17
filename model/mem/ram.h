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

extern bool ram_read(ram_t *ram, u32 addr, u32 *data);
extern bool ram_write(ram_t *ram, u32 addr, u32 data, u8 strobe);
extern bus_rsp_t ram_bus_req_handler(ram_t *ram, u32 base_addr, const bus_req_t *req);

#endif