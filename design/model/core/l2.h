#ifndef L2_H
#define L2_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/axi4_if.h"
#include "bus/mux/axi_mux.h"

#define L2_HOST_NUM 2u
#define L2_LINE_SIZE 64u

typedef struct l2_conf {
    u32 size;
    u32 way_num;
} l2_conf_t;

typedef enum l2_state {
    L2_STATE_IDLE = 0,
    L2_STATE_READ_SERVE,
    L2_STATE_WRITE_WAIT_W,
    L2_STATE_WRITE_SERVE,
    L2_STATE_WB_AW,
    L2_STATE_WB_W,
    L2_STATE_WB_B,
    L2_STATE_REFILL_AR,
    L2_STATE_REFILL_R,
    L2_STATE_BYPASS_R,
    L2_STATE_BYPASS_W,
    L2_STATE_BYPASS_B,
} l2_state_t;

typedef struct l2 {
    mod_t mod;

    AXI4_SLV_ARR_DECL(host_, L2_HOST_NUM);
    AXI4_MST_DECL(gst_);

    axi_mux_t axi_mux;
    AXI4_IF_DECL(merged_);

    l2_conf_t conf;
    u32 set_num;
    u32 line_num;
    u32 *tags;
    u32 *data;
    u32 *replace_ways;
    bool *valids;
    bool *dirtys;

    l2_state_t state;
    bool is_write;
    axi4_ar_if_t ar;
    axi4_aw_if_t aw;
    axi4_w_if_t w;
    u32 burst_addr;
    u32 beat_idx;
    u32 byte_idx;
    u32 beat_data;
    u8 write_resp;
    u8 beat_resp;

    u32 req_line_addr;
    u32 req_set;
    u32 req_way;
    u32 req_line_idx;
    u32 req_tag;
    u32 wb_line_addr;
    u32 line_beat_idx;

    u64 *perf_hit;
    u64 *perf_miss;
    u64 *perf_bypass;
    u64 *perf_writeback;
} l2_t;

extern void l2_construct(l2_t *l2, const char *parent, const char *name,
    const l2_conf_t *conf);
extern void l2_reset(l2_t *l2);
extern void l2_clock(l2_t *l2);
extern void l2_free(l2_t *l2);

#endif
