#ifndef BRIDGE_H
#define BRIDGE_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"

typedef struct bti2apb {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;
    itf_t *apb_req_mst;
    itf_t *apb_rsp_slv;
    u32 bti_trans_id;
} bti2apb_t;

extern void bti2apb_construct(bti2apb_t *br);
extern void bti2apb_reset(bti2apb_t *br);
extern void bti2apb_clock(bti2apb_t *br);
extern void bti2apb_free(bti2apb_t *br);

#endif