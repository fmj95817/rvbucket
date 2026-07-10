#ifndef L1_H
#define L1_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/bti_if.h"
#include "itf/axi4_if.h"
#include "itf/l1_flush_if.h"

#define L1_BYPASS_RANGE_NUM 8
#define L1_LINE_SIZE 64u

typedef struct l1_conf {
    bool full_bypass;
    bool ro;
    u32 size;
    u32 way_num;
    u32 stg_fifo_depth;
    u32 ost_depth;
    u32 bypass_bases[L1_BYPASS_RANGE_NUM];
    u32 bypass_sizes[L1_BYPASS_RANGE_NUM];
} l1_conf_t;

typedef enum l1_state {
    L1_STATE_IDLE = 0,
    L1_STATE_WB_AW,
    L1_STATE_WB_W,
    L1_STATE_WB_B,
    L1_STATE_REFILL_AR,
    L1_STATE_REFILL_R,
    L1_STATE_SERVE_MISS,
} l1_state_t;

typedef enum l1_ost_kind {
    L1_OST_CACHED = 0,
    L1_OST_BYPASS_RD,
    L1_OST_BYPASS_WR,
} l1_ost_kind_t;

typedef struct l1_ost_ctx {
    u32 trans_id;
    l1_ost_kind_t kind;
    bool rsp_vld;
    bool ok;
    u32 data;
} l1_ost_ctx_t;

typedef struct l1 {
    mod_t mod;

    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;

    itf_t *axi4_aw_mst;
    itf_t *axi4_w_mst;
    itf_t *axi4_b_slv;
    itf_t *axi4_ar_mst;
    itf_t *axi4_r_slv;
    itf_t *flush_slv;

    l1_conf_t conf;
    u32 set_num;
    u32 line_num;
    u32 *tags;
    u32 *data;
    u32 *replace_ways;
    bool *valids;
    bool *dirtys;

    fifo_t req_fifo;
    ostq_t ost;

    l1_state_t state;
    u32 active_slot;
    bti_req_if_t req;
    u32 req_set;
    u32 req_way;
    u32 req_line_idx;
    u32 req_tag;
    u32 req_line_addr;
    u32 req_byte_idx;
    u32 req_data;
    u32 wb_line_addr;
    u32 beat_idx;
    bool op_ok;

    u64 *perf_hit;
    u64 *perf_miss;
    u64 *perf_bypass;
    u64 *perf_writeback;
    u64 *perf_stg_full;
} l1_t;

extern void l1_construct(l1_t *l1, const char *parent, const char *name, const l1_conf_t *conf);
extern void l1_reset(l1_t *l1);
extern void l1_clock(l1_t *l1);
extern void l1_free(l1_t *l1);

#endif
