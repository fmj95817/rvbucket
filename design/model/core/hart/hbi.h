#ifndef HBI_H
#define HBI_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/ldst_req_if.h"
#include "itf/ldst_rsp_if.h"
#include "itf/bti_if.h"
#include "itf/hart_expt_if.h"

typedef struct hbi_conf {
    u32 stg_fifo_depth;
    u32 i_ost_depth;
    u32 d_ost_depth;
} hbi_conf_t;

typedef struct hbi_i_ost_ctx {
    u32 pc;
    bool rsp_vld;
    fch_rsp_if_t rsp;
} hbi_i_ost_ctx_t;

typedef struct hbi_d_ost_ctx {
    bool rsp_vld;
    ldst_rsp_if_t rsp;
} hbi_d_ost_ctx_t;

typedef struct hbi {
    mod_t mod;
    itf_t *i_bti_req_mst;
    itf_t *i_bti_rsp_slv;
    itf_t *d_bti_req_mst;
    itf_t *d_bti_rsp_slv;

    itf_t *fch_req_slv;
    itf_t *fch_rsp_mst;
    itf_t *mmu_fch_expt_slv;
    itf_t *ldst_req_slv;
    itf_t *ldst_rsp_mst;

    fifo_t fch_req_fifo;
    fifo_t ldst_req_fifo;
    ostq_t i_ost;
    ostq_t d_ost;

    u64 *perf_i_stg_full;
    u64 *perf_d_stg_full;
} hbi_t;

extern void hbi_construct(hbi_t *hbi, const char *parent, const char *name,
    const hbi_conf_t *conf);
extern void hbi_reset(hbi_t *hbi);
extern void hbi_clock(hbi_t *hbi);
extern void hbi_free(hbi_t *hbi);

#endif
