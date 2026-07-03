#ifndef SRAM_H
#define SRAM_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/bti_if.h"
#include "itf/axi4_if.h"

#define RAM_MAX_PORT_NUM 2

typedef enum ram_mode {
    RAM_MODE_BTI = 0,
    RAM_MODE_AXI = 1
} ram_mode_t;

typedef struct ram {
    mod_t mod;
    itf_t *bti_req_slvs[RAM_MAX_PORT_NUM];
    itf_t *bti_rsp_msts[RAM_MAX_PORT_NUM];

    itf_t *axi4_aw_slv;
    itf_t *axi4_w_slv;
    itf_t *axi4_b_mst;
    itf_t *axi4_ar_slv;
    itf_t *axi4_r_mst;

    u32 port_num;
    ram_mode_t mode;
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

    u32 wr_burst_addr;
    u8 wr_burst_id;
    u8 wr_burst_len;
    axi4_aw_size_t wr_burst_size;
    axi4_aw_burst_t wr_burst_type;
    u8 wr_burst_cnt;
    bool wr_active;
    bool wr_b_pending;
} ram_t;

extern void ram_construct(ram_t *ram, const char *parent, const char *name, u32 port_num, ram_mode_t mode,
                          u32 size, u32 base_addr);
extern void ram_reset(ram_t *ram);
extern void ram_clock(ram_t *ram);
extern void ram_free(ram_t *ram);
extern void ram_load(ram_t *ram, const void *data, u32 addr, u32 size);

#endif
