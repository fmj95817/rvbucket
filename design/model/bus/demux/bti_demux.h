#ifndef BTI_DEMUX_H
#define BTI_DEMUX_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/bti_if.h"

#define BTI_DEMUX_GST_NUM_MAX 16

typedef struct bti_demux_conf {
    u32 gst_num;
    const u32 *gst_bases;
    const u32 *gst_sizes;
    u32 stg_fifo_depth;
    u32 ost_depth;
} bti_demux_conf_t;

typedef struct bti_demux_ost_ctx {
    u32 gst_idx;
} bti_demux_ost_ctx_t;

typedef struct bti_demux {
    mod_t mod;
    itf_t *host_bti_req_slv;
    itf_t *host_bti_rsp_mst;
    itf_t *gst_bti_req_msts[BTI_DEMUX_GST_NUM_MAX];
    itf_t *gst_bti_rsp_slvs[BTI_DEMUX_GST_NUM_MAX];

    u32 gst_num;
    u32 gst_bases[BTI_DEMUX_GST_NUM_MAX];
    u32 gst_sizes[BTI_DEMUX_GST_NUM_MAX];
    fifo_t req_fifo;
    ostk_t ost;

    u64 *perf_stg_full;
} bti_demux_t;

extern void bti_demux_construct(bti_demux_t *bti_demux, const char *parent, const char *name,
    const bti_demux_conf_t *conf);
extern void bti_demux_reset(bti_demux_t *bti_demux);
extern void bti_demux_clock(bti_demux_t *bti_demux);
extern void bti_demux_free(bti_demux_t *bti_demux);

#endif
