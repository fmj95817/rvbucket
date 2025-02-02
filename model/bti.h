#ifndef BTI_H
#define BTI_H

#include "base/types.h"
#include "base/itf.h"

typedef enum bti_cmd {
    BTI_CMD_READ = 0,
    BTI_CMD_WRITE = 1
} bti_cmd_t;

typedef struct bti_req_if {
    u32 trans_id;
    bti_cmd_t cmd;
    u32 addr;
    u32 data;
    u8 strobe;
} bti_req_if_t;

typedef struct bti_rsp_if {
    u32 trans_id;
    u32 data;
    bool ok;
} bti_rsp_if_t;

typedef struct bti_mux {
    itf_t *host_bti_req_slv;
    itf_t *host_bti_rsp_mst;
    itf_t **gst_bti_req_msts;
    itf_t **gst_bti_rsp_slvs;

    u32 gst_num;
    u32 *gst_base_addrs;
    u32 *gst_sizes;
} bti_mux_t;

extern void bti_mux_construct(bti_mux_t *bti_mux, u32 gst_num, const u32 *gst_base_addrs, const u32 *gst_sizes);
extern void bti_mux_reset(bti_mux_t *bti_mux);
extern void bti_mux_clock(bti_mux_t *bti_mux);
extern void bti_mux_free(bti_mux_t *bti_mux);

#endif