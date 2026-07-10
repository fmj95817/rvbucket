#ifndef AXI_DEMUX_H
#define AXI_DEMUX_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/axi4_if.h"

#define AXI_DEMUX_GST_NUM_MAX 16

typedef struct axi_demux_conf {
    u32 gst_num;
    const u32 *gst_bases;
    const u32 *gst_sizes;
    u32 stg_fifo_depth;
    u32 ost_depth;
} axi_demux_conf_t;

typedef struct axi_demux_rd_ctx {
    u32 gst_idx;
} axi_demux_rd_ctx_t;

typedef struct axi_demux_wr_rsp_ctx {
    u32 gst_idx;
    bool decerr;
    bool decerr_valid;
} axi_demux_wr_rsp_ctx_t;

typedef struct axi_demux_wr_data_ctx {
    u32 gst_idx;
    u32 rsp_slot;
    bool decerr;
} axi_demux_wr_data_ctx_t;

typedef struct axi_demux {
    mod_t mod;
    itf_t *host_axi4_aw_slv;
    itf_t *host_axi4_w_slv;
    itf_t *host_axi4_b_mst;
    itf_t *host_axi4_ar_slv;
    itf_t *host_axi4_r_mst;

    itf_t *gst_axi4_aw_msts[AXI_DEMUX_GST_NUM_MAX];
    itf_t *gst_axi4_w_msts[AXI_DEMUX_GST_NUM_MAX];
    itf_t *gst_axi4_b_slvs[AXI_DEMUX_GST_NUM_MAX];
    itf_t *gst_axi4_ar_msts[AXI_DEMUX_GST_NUM_MAX];
    itf_t *gst_axi4_r_slvs[AXI_DEMUX_GST_NUM_MAX];

    u32 gst_num;
    u32 gst_bases[AXI_DEMUX_GST_NUM_MAX];
    u32 gst_sizes[AXI_DEMUX_GST_NUM_MAX];

    u32 rd_rsp_rr_idx;
    u32 wr_rsp_rr_idx;
    fifo_t ar_fifo;
    fifo_t aw_fifo;
    fifo_t w_fifo;
    ostk_t rd_ost;
    ostq_t wr_data_ost;
    ostk_t wr_rsp_ost;

    u64 *perf_stg_ar_full;
    u64 *perf_stg_aw_full;
    u64 *perf_stg_w_full;
} axi_demux_t;

extern void axi_demux_construct(axi_demux_t *axi_demux, const char *parent, const char *name,
    const axi_demux_conf_t *conf);
extern void axi_demux_reset(axi_demux_t *axi_demux);
extern void axi_demux_clock(axi_demux_t *axi_demux);
extern void axi_demux_free(axi_demux_t *axi_demux);

#endif
