#ifndef ROM_H
#define ROM_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/axi4_aw_if.h"
#include "itf/axi4_w_if.h"
#include "itf/axi4_b_if.h"
#include "itf/axi4_ar_if.h"
#include "itf/axi4_r_if.h"

typedef enum rom_mode {
    ROM_MODE_BTI = 0,
    ROM_MODE_AXI = 1
} rom_mode_t;

typedef struct rom {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;

    itf_t *axi4_aw_slv;
    itf_t *axi4_w_slv;
    itf_t *axi4_b_mst;
    itf_t *axi4_ar_slv;
    itf_t *axi4_r_mst;

    rom_mode_t mode;
    u32 base_addr;
    u32 size;
    u8 *data;

    u32 rd_burst_addr;
    u8 rd_burst_id;
    u8 rd_burst_len;
    axi4_ar_size_t rd_burst_size;
    axi4_ar_burst_t rd_burst_type;
    u8 rd_burst_cnt;
    bool rd_active;
} rom_t;

extern void rom_construct(rom_t *rom, const char *name, rom_mode_t mode,
    u32 size, const void *data, u32 data_size, u32 base_addr);
extern void rom_reset(rom_t *rom);
extern void rom_clock(rom_t *rom);
extern void rom_free(rom_t *rom);

extern void rom_burn(rom_t *rom, const void *data, u32 addr, u32 size);

#endif
