#ifndef MUX_H
#define MUX_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"

#define BTI_MUX_HOST_NUM_MAX 16

typedef struct bti_mux {
    itf_t *host_bti_req_slvs[BTI_MUX_HOST_NUM_MAX];
    itf_t *host_bti_rsp_msts[BTI_MUX_HOST_NUM_MAX];
    itf_t *gst_bti_req_mst;
    itf_t *gst_bti_rsp_slv;
    u32 host_num;
    bool rsp_pending;
    u32 rsp_host_idx;
} bti_mux_t;

extern void bti_mux_construct(bti_mux_t *bti_mux, const char *name, u32 host_num);
extern void bti_mux_reset(bti_mux_t *bti_mux);
extern void bti_mux_clock(bti_mux_t *bti_mux);
extern void bti_mux_free(bti_mux_t *bti_mux);

#endif
