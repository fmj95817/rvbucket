#ifndef BTI_MUX_H
#define BTI_MUX_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/bti_if.h"

#define BTI_MUX_HOST_NUM_MAX 16

typedef struct bti_mux_conf {
    u32 host_num;
    u32 stg_fifo_depth;
    u32 ost_depth;
} bti_mux_conf_t;

typedef struct bti_mux_ost_ctx {
    u32 host_idx;
} bti_mux_ost_ctx_t;

typedef struct bti_mux {
    mod_t mod;
    itf_t *host_bti_req_slvs[BTI_MUX_HOST_NUM_MAX];
    itf_t *host_bti_rsp_msts[BTI_MUX_HOST_NUM_MAX];
    itf_t *gst_bti_req_mst;
    itf_t *gst_bti_rsp_slv;
    u32 host_num;
    u32 req_rr_idx;
    fifo_t host_req_fifos[BTI_MUX_HOST_NUM_MAX];
    ostk_t ost;

    u64 *perf_stg_full;
} bti_mux_t;

extern void bti_mux_construct(bti_mux_t *bti_mux, const char *parent, const char *name,
    const bti_mux_conf_t *conf);
extern void bti_mux_reset(bti_mux_t *bti_mux);
extern void bti_mux_clock(bti_mux_t *bti_mux);
extern void bti_mux_free(bti_mux_t *bti_mux);

#endif
