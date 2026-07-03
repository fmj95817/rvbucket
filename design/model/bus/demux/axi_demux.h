#ifndef AXI_DEMUX_H
#define AXI_DEMUX_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/axi4_if.h"

#define AXI_DEMUX_GST_NUM_MAX 16

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

    u32 rd_state;
    u32 rd_active_gst_idx;

    u32 wr_state;
    u32 wr_active_gst_idx;
    u8 wr_decerr_id;
} axi_demux_t;

extern void axi_demux_construct(axi_demux_t *axi_demux, const char *parent, const char *name,
    u32 gst_num, const u32 *gst_bases, const u32 *gst_sizes);
extern void axi_demux_reset(axi_demux_t *axi_demux);
extern void axi_demux_clock(axi_demux_t *axi_demux);
extern void axi_demux_free(axi_demux_t *axi_demux);

#endif
