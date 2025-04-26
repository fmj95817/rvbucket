#ifndef DEMUX_H
#define DEMUX_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"

#define BTI_MUX_GST_NUM_MAX 16

typedef struct bti_demux {
    itf_t *host_bti_req_slv;
    itf_t *host_bti_rsp_mst;
    itf_t *gst_bti_req_msts[BTI_MUX_GST_NUM_MAX];
    itf_t *gst_bti_rsp_slvs[BTI_MUX_GST_NUM_MAX];

    u32 gst_num;
    u32 gst_base_addrs[BTI_MUX_GST_NUM_MAX];
    u32 gst_sizes[BTI_MUX_GST_NUM_MAX];
} bti_demux_t;

extern void bti_demux_construct(bti_demux_t *bti_demux, u32 gst_num, const u32 *gst_base_addrs, const u32 *gst_sizes);
extern void bti_demux_reset(bti_demux_t *bti_demux);
extern void bti_demux_clock(bti_demux_t *bti_demux);
extern void bti_demux_free(bti_demux_t *bti_demux);

#endif