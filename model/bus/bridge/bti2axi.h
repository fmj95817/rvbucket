#ifndef BTI2AXI4_H
#define BTI2AXI4_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/axi4_aw_if.h"
#include "itf/axi4_w_if.h"
#include "itf/axi4_b_if.h"
#include "itf/axi4_ar_if.h"
#include "itf/axi4_r_if.h"

typedef enum bti2axi_state {
    BTI2AXI4_STATE_IDLE = 0,
    BTI2AXI4_STATE_RD_PENDING = 1,
    BTI2AXI4_STATE_WR_PENDING = 2,
} bti2axi_state_t;

typedef struct bti2axi {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;
    itf_t *axi4_aw_mst;
    itf_t *axi4_w_mst;
    itf_t *axi4_b_slv;
    itf_t *axi4_ar_mst;
    itf_t *axi4_r_slv;
    bti2axi_state_t state;
    u16 bti_trans_id;
} bti2axi_t;

extern void bti2axi_construct(bti2axi_t *br, const char *name);
extern void bti2axi_reset(bti2axi_t *br);
extern void bti2axi_clock(bti2axi_t *br);
extern void bti2axi_free(bti2axi_t *br);

#endif
