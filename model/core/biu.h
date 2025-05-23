#ifndef BIU_H
#define BIU_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"

typedef struct biu {
    itf_t *i_bti_req_mst;
    itf_t *i_bti_rsp_slv;

    itf_t *d_bti_req_mst;
    itf_t *d_bti_rsp_slv;

    itf_t *fch_req_slv;
    itf_t *fch_rsp_mst;

    itf_t *ldst_req_slv;
    itf_t *ldst_rsp_mst;
} biu_t;

extern void biu_construct(biu_t *biu);
extern void biu_reset(biu_t *biu);
extern void biu_clock(biu_t *biu);
extern void biu_free(biu_t *biu);

#endif