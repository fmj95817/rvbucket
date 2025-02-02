#ifndef SRAM_H
#define SRAM_H

#include "base/types.h"
#include "base/itf.h"
#include "bti.h"

typedef struct ram {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;

    u32 base_addr;
    u32 size;
    u8 *data;
} ram_t;

extern void ram_construct(ram_t *ram, u32 size, u32 base_addr);
extern void ram_reset(ram_t *ram);
extern void ram_clock(ram_t *ram);
extern void ram_free(ram_t *ram);

#endif