#ifndef LSU_H
#define LSU_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/csr_lsu_state_if.h"

typedef struct lsu {
    mod_t mod;
    itf_t *exu_ldst_req_slv;
    itf_t *exu_ldst_rsp_mst;
    itf_t *hbi_ldst_req_mst;
    itf_t *hbi_ldst_rsp_slv;
    itf_t *csr_lsu_state_in;

    const csr_lsu_state_if_t *csr_state_i;

    bool busy;
    bool split;
    ldst_req_if_t req;
    u32 byte_num;
    u32 req_byte_idx;
    u32 rsp_byte_idx;
    u32 rsp_data;
} lsu_t;

extern void lsu_construct(lsu_t *lsu, const char *parent, const char *name);
extern void lsu_reset(lsu_t *lsu);
extern void lsu_clock(lsu_t *lsu);
extern void lsu_free(lsu_t *lsu);

#endif
