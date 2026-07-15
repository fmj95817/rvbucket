#ifndef BTI2AXI4_H
#define BTI2AXI4_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/bti_if.h"
#include "itf/axi4_if.h"

#define BTI2AXI_AXI_ID 0

typedef struct bti2axi_conf {
    u32 stg_fifo_depth;
    u32 ost_depth;
} bti2axi_conf_t;

typedef struct bti2axi_ost_ctx {
    u16 bti_trans_id;
} bti2axi_ost_ctx_t;

typedef struct bti2axi {
    mod_t mod;
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;
    itf_t *axi4_aw_mst;
    itf_t *axi4_w_mst;
    itf_t *axi4_b_slv;
    itf_t *axi4_ar_mst;
    itf_t *axi4_r_slv;
    fifo_t bti_req_fifo;
    ostk_t rd_ost;
    ostk_t wr_ost;

    u64 *perf_stg_full;
} bti2axi_t;

extern void bti2axi_construct(bti2axi_t *br, const char *parent, const char *name,
    const bti2axi_conf_t *conf);
extern void bti2axi_reset(bti2axi_t *br);
extern void bti2axi_clock(bti2axi_t *br);
extern void bti2axi_free(bti2axi_t *br);

#endif
