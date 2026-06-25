#ifndef AXI_MUX_H
#define AXI_MUX_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/axi4_aw_if.h"
#include "itf/axi4_w_if.h"
#include "itf/axi4_b_if.h"
#include "itf/axi4_ar_if.h"
#include "itf/axi4_r_if.h"

#define AXI_MUX_HOST_NUM_MAX 16

#define AXI_MUX_STATE_IDLE 0
#define AXI_MUX_STATE_RD 1
#define AXI_MUX_STATE_WR_DATA 2
#define AXI_MUX_STATE_WR_RESP 3

typedef struct axi_mux {
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

    u32 rd_state;
    u32 rd_active_host_idx;
    u32 rd_rr_idx;

    u32 wr_state;
    u32 wr_active_host_idx;
    u32 wr_rr_idx;
    bool wr_w_done;
} axi_mux_t;

extern void axi_mux_construct(axi_mux_t *axi_mux, const char *name, u32 host_num);
extern void axi_mux_reset(axi_mux_t *axi_mux);
extern void axi_mux_clock(axi_mux_t *axi_mux);
extern void axi_mux_free(axi_mux_t *axi_mux);

#endif
