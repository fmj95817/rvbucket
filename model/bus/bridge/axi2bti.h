#ifndef AXI2BTI_H
#define AXI2BTI_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/axi4_aw_if.h"
#include "itf/axi4_w_if.h"
#include "itf/axi4_b_if.h"
#include "itf/axi4_ar_if.h"
#include "itf/axi4_r_if.h"

typedef enum axi2bti_state {
    AXI2BTI_STATE_IDLE = 0,
    AXI2BTI_STATE_RD_BURST = 1,
    AXI2BTI_STATE_WR_BURST = 2,
} axi2bti_state_t;

typedef struct axi2bti {
    itf_t *axi4_ar_slv;
    itf_t *axi4_r_mst;
    itf_t *axi4_aw_slv;
    itf_t *axi4_w_slv;
    itf_t *axi4_b_mst;
    itf_t *bti_req_mst;
    itf_t *bti_rsp_slv;
    axi2bti_state_t state;

    u8  axid;
    u32 axaddr;
    u8  axlen;
    u8  axsize;
    u8  axburst;
    u8  beat_idx;
    bool bti_rsp_pending;
    u16 next_bti_trans_id;
} axi2bti_t;

extern void axi2bti_construct(axi2bti_t *br, const char *name);
extern void axi2bti_reset(axi2bti_t *br);
extern void axi2bti_clock(axi2bti_t *br);
extern void axi2bti_free(axi2bti_t *br);

#endif
