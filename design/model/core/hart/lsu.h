#ifndef LSU_H
#define LSU_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/csr_lsu_state_if.h"

typedef struct lsu_conf {
    u32 stg_fifo_depth;
    u32 ost_depth;
} lsu_conf_t;

typedef struct lsu_ost_ctx {
    ldst_req_if_t req;
    bool split;
    u32 byte_num;
    u32 req_idx;
    u32 rsp_idx;
    u32 rsp_data;
    bool ready;
    bool ok;
} lsu_ost_ctx_t;

typedef struct lsu {
    mod_t mod;
    itf_t *exu_ldst_req_slv;
    itf_t *exu_ldst_rsp_mst;
    itf_t *hbi_ldst_req_mst;
    itf_t *hbi_ldst_rsp_slv;
    itf_t *csr_lsu_state_in;

    const csr_lsu_state_if_t *csr_state_i;

    fifo_t req_fifo;
    ostq_t ost;

    bool busy;
    bool split;
    ldst_req_if_t req;
    u32 byte_num;
    u32 req_byte_idx;
    u32 rsp_byte_idx;
    u32 rsp_data;

    u64 *perf_stg_full;
} lsu_t;

extern void lsu_construct(lsu_t *lsu, const char *parent, const char *name,
    const lsu_conf_t *conf);
extern void lsu_reset(lsu_t *lsu);
extern void lsu_clock(lsu_t *lsu);
extern void lsu_free(lsu_t *lsu);

#endif
