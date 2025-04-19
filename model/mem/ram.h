#ifndef SRAM_H
#define SRAM_H

#include "base/types.h"
#include "base/itf.h"
#include "bti.h"

#define RAM_MAX_PORT_NUM 2

typedef struct ram {
    itf_t *bti_req_slv[RAM_MAX_PORT_NUM];
    itf_t *bti_rsp_mst[RAM_MAX_PORT_NUM];

    u32 port_num;
    u32 base_addr;
    u32 size;
    u8 *data;
} ram_t;

extern void ram_construct(ram_t *ram, u32 port_num, u32 size, u32 base_addr);
extern void ram_reset(ram_t *ram);
extern void ram_clock(ram_t *ram);
extern void ram_free(ram_t *ram);
extern void ram_load(ram_t *ram, const void *data, u32 addr, u32 size);

#endif