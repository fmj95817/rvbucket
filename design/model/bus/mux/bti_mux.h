#ifndef BTI_MUX_H
#define BTI_MUX_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/bti_if.h"

#define BTI_MUX_HOST_NUM_MAX 16

typedef struct bti_mux {
    mod_t mod;
    itf_t *host_bti_req_slvs[BTI_MUX_HOST_NUM_MAX];
    itf_t *host_bti_rsp_msts[BTI_MUX_HOST_NUM_MAX];
    itf_t *gst_bti_req_mst;
    itf_t *gst_bti_rsp_slv;
    u32 host_num;
    bool rsp_pending;
    u32 rsp_host_idx;
    u32 req_rr_idx;
} bti_mux_t;

extern void bti_mux_construct(bti_mux_t *bti_mux, const char *parent, const char *name, u32 host_num);
extern void bti_mux_reset(bti_mux_t *bti_mux);
extern void bti_mux_clock(bti_mux_t *bti_mux);
extern void bti_mux_free(bti_mux_t *bti_mux);

#endif
