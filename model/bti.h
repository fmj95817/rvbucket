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

static inline void bti_req_if_to_str(const void *pkt, char *str)
{
    const bti_req_if_t *bti_req = (const bti_req_if_t *)pkt;
    sprintf(str, "%u %d %x %x %x\n", bti_req->trans_id,
        bti_req->cmd, bti_req->addr, bti_req->data, bti_req->strobe);
}

static inline void bti_rsp_if_to_str(const void *pkt, char *str)
{
    const bti_rsp_if_t *bti_rsp = (const bti_rsp_if_t *)pkt;
    sprintf(str, "%u %x %d\n", bti_rsp->trans_id, bti_rsp->data, bti_rsp->ok);
}

#define BTI_MUX_GST_NUM_MAX 16

typedef struct bti_mux {
    itf_t *host_bti_req_slv;
    itf_t *host_bti_rsp_mst;
    itf_t *gst_bti_req_msts[BTI_MUX_GST_NUM_MAX];
    itf_t *gst_bti_rsp_slvs[BTI_MUX_GST_NUM_MAX];

    u32 gst_num;
    u32 gst_base_addrs[BTI_MUX_GST_NUM_MAX];
    u32 gst_sizes[BTI_MUX_GST_NUM_MAX];
} bti_mux_t;

extern void bti_mux_construct(bti_mux_t *bti_mux, u32 gst_num, const u32 *gst_base_addrs, const u32 *gst_sizes);
extern void bti_mux_reset(bti_mux_t *bti_mux);
extern void bti_mux_clock(bti_mux_t *bti_mux);
extern void bti_mux_free(bti_mux_t *bti_mux);

#endif