#ifndef BIU_H
#define BIU_H

#include "base/types.h"
#include "base/itf.h"
#include "base/arb.h"

typedef struct biu {
    itf_t *bti_req_mst;
    itf_t *bti_rsp_slv;

    itf_t *fch_req_slv;
    itf_t *fch_rsp_mst;

    itf_t *ldst_req_slv;
    itf_t *ldst_rsp_mst;

    arb_t rr_arb;
} biu_t;

extern void biu_construct(biu_t *biu);
extern void biu_reset(biu_t *biu);
extern void biu_clock(biu_t *biu);
extern void biu_free(biu_t *biu);

#endif