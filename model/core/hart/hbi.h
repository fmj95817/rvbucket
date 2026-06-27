#ifndef HBI_H
#define HBI_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"
#include "itf/hart_expt_if.h"

typedef struct hbi {
    itf_t *i_bti_req_mst;
    itf_t *i_bti_rsp_slv;
    itf_t *d_bti_req_mst;
    itf_t *d_bti_rsp_slv;

    itf_t *fch_req_slv;
    itf_t *fch_rsp_mst;
    itf_t *i_hart_expt_slv;
    itf_t *ldst_req_slv;
    itf_t *ldst_rsp_mst;
} hbi_t;

extern void hbi_construct(hbi_t *hbi, const char *name);
extern void hbi_reset(hbi_t *hbi);
extern void hbi_clock(hbi_t *hbi);
extern void hbi_free(hbi_t *hbi);

#endif
