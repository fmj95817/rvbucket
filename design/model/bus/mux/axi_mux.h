#ifndef AXI_MUX_H
#define AXI_MUX_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/axi4_if.h"

#define AXI_MUX_HOST_NUM_MAX 16

typedef struct axi_mux_conf {
    u32 host_num;
    u32 stg_fifo_depth;
    u32 ost_depth;
} axi_mux_conf_t;

typedef struct axi_mux_ost_ctx {
    u32 host_idx;
} axi_mux_ost_ctx_t;

typedef struct axi_mux {
    mod_t mod;
    itf_t *host_axi4_aw_slvs[AXI_MUX_HOST_NUM_MAX];
    itf_t *host_axi4_w_slvs[AXI_MUX_HOST_NUM_MAX];
    itf_t *host_axi4_b_msts[AXI_MUX_HOST_NUM_MAX];
    itf_t *host_axi4_ar_slvs[AXI_MUX_HOST_NUM_MAX];
    itf_t *host_axi4_r_msts[AXI_MUX_HOST_NUM_MAX];

    itf_t *gst_axi4_aw_mst;
    itf_t *gst_axi4_w_mst;
    itf_t *gst_axi4_b_slv;
    itf_t *gst_axi4_ar_mst;
    itf_t *gst_axi4_r_slv;

    u32 host_num;

    u32 rd_rr_idx;
    u32 wr_rr_idx;
    fifo_t host_ar_fifos[AXI_MUX_HOST_NUM_MAX];
    fifo_t host_aw_fifos[AXI_MUX_HOST_NUM_MAX];
    fifo_t host_w_fifos[AXI_MUX_HOST_NUM_MAX];
    ostk_t rd_ost;
    ostq_t wr_data_ost;
    ostk_t wr_rsp_ost;

    u64 *perf_stg_ar_full;
    u64 *perf_stg_aw_full;
    u64 *perf_stg_w_full;
} axi_mux_t;

extern void axi_mux_construct(axi_mux_t *axi_mux, const char *parent, const char *name,
    const axi_mux_conf_t *conf);
extern void axi_mux_reset(axi_mux_t *axi_mux);
extern void axi_mux_clock(axi_mux_t *axi_mux);
extern void axi_mux_free(axi_mux_t *axi_mux);

#endif
