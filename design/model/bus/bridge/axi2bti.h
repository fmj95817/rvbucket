#ifndef AXI2BTI_H
#define AXI2BTI_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/bti_if.h"
#include "itf/axi4_if.h"

typedef struct axi2bti_conf {
    u32 stg_fifo_depth;
    u32 ost_depth;
} axi2bti_conf_t;

typedef enum axi2bti_beat_kind {
    AXI2BTI_BEAT_READ = 0,
    AXI2BTI_BEAT_WRITE = 1,
} axi2bti_beat_kind_t;

typedef struct axi2bti_rd_ctx {
    u8 axid;
    u32 axaddr;
    u8 axlen;
    u8 axsize;
    u8 axburst;
    u8 beat_idx;
    bool bti_rsp_pending;
    bool rsp_valid;
    bool rsp_ok;
    u32 rsp_data;
} axi2bti_rd_ctx_t;

typedef struct axi2bti_wr_ctx {
    u8 axid;
    u32 axaddr;
    u8 axlen;
    u8 axsize;
    u8 axburst;
    u8 beat_idx;
    bool bti_rsp_pending;
    bool pending_last;
    bool b_valid;
    bool b_ok;
} axi2bti_wr_ctx_t;

typedef struct axi2bti_beat_ctx {
    axi2bti_beat_kind_t kind;
    u32 slot;
} axi2bti_beat_ctx_t;

typedef struct axi2bti {
    mod_t mod;
    itf_t *axi4_ar_slv;
    itf_t *axi4_r_mst;
    itf_t *axi4_aw_slv;
    itf_t *axi4_w_slv;
    itf_t *axi4_b_mst;
    itf_t *bti_req_mst;
    itf_t *bti_rsp_slv;
    fifo_t ar_fifo;
    fifo_t aw_fifo;
    fifo_t w_fifo;
    ostk_t rd_ost;
    ostq_t wr_ost;
    ostk_t beat_ost;
    u16 next_bti_trans_id;
    bool issue_wr_prio;

    u64 *perf_stg_ar_full;
    u64 *perf_stg_aw_full;
    u64 *perf_stg_w_full;
} axi2bti_t;

extern void axi2bti_construct(axi2bti_t *br, const char *parent, const char *name,
    const axi2bti_conf_t *conf);
extern void axi2bti_reset(axi2bti_t *br);
extern void axi2bti_clock(axi2bti_t *br);
extern void axi2bti_free(axi2bti_t *br);

#endif
