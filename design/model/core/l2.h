#ifndef L2_H
#define L2_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/axi4_if.h"

#define L2_LINE_SIZE 64u
#define L2_PORT_NUM 2u
#define L2_I_PORT 0u
#define L2_D_PORT 1u

typedef struct l2_conf {
    u32 size;
    u32 way_num;
    u32 latency;
    u32 stg_fifo_depth;
    u32 bypass_ost_depth;
} l2_conf_t;

typedef enum l2_port_state {
    L2_PORT_STATE_IDLE = 0,
    L2_PORT_STATE_READ,
    L2_PORT_STATE_WRITE,
    L2_PORT_STATE_MISS_WAIT,
    L2_PORT_STATE_CMO_WAIT,
    L2_PORT_STATE_CMO_RSP,
} l2_port_state_t;

typedef struct l2_port {
    l2_port_state_t state;
    l2_port_state_t miss_return_state;
    axi4_ar_if_t ar;
    axi4_aw_if_t aw;
    u32 addr;
    u32 beat_idx;
    bool miss_resolved;
    bool latency_active;
    u32 delay;
} l2_port_t;

typedef struct l2_bypass_rd_ctx {
    u32 port;
} l2_bypass_rd_ctx_t;

typedef struct l2_bypass_wr_ctx {
    u32 port;
    u32 beat_idx;
    u32 beat_num;
    bool w_done;
} l2_bypass_wr_ctx_t;

typedef enum l2_mem_state {
    L2_MEM_STATE_IDLE = 0,
    L2_MEM_STATE_WB_AW,
    L2_MEM_STATE_WB_W,
    L2_MEM_STATE_WB_B,
    L2_MEM_STATE_REFILL_AR,
    L2_MEM_STATE_REFILL_R,
    L2_MEM_STATE_CMO_WB_AW,
    L2_MEM_STATE_CMO_WB_W,
    L2_MEM_STATE_CMO_WB_B,
} l2_mem_state_t;

typedef struct l2 {
    mod_t mod;

    itf_t *i_axi4_aw_slv;
    itf_t *i_axi4_w_slv;
    itf_t *i_axi4_b_mst;
    itf_t *i_axi4_ar_slv;
    itf_t *i_axi4_r_mst;

    itf_t *d_axi4_aw_slv;
    itf_t *d_axi4_w_slv;
    itf_t *d_axi4_b_mst;
    itf_t *d_axi4_ar_slv;
    itf_t *d_axi4_r_mst;

    itf_t *mem_axi4_aw_mst;
    itf_t *mem_axi4_w_mst;
    itf_t *mem_axi4_b_slv;
    itf_t *mem_axi4_ar_mst;
    itf_t *mem_axi4_r_slv;

    l2_conf_t conf;
    u32 set_num;
    u32 line_num;
    u32 *tags;
    u32 *data;
    u32 *replace_ways;
    bool *valids;
    bool *dirtys;

    fifo_t ar_fifos[L2_PORT_NUM];
    fifo_t aw_fifos[L2_PORT_NUM];
    fifo_t w_fifos[L2_PORT_NUM];
    ostq_t bypass_rd_ost;
    ostq_t bypass_wr_ost;

    l2_port_t ports[L2_PORT_NUM];
    l2_mem_state_t mem_state;
    u32 mem_owner;
    u32 miss_line_addr;
    u32 miss_set;
    u32 miss_tag;
    u32 miss_line_idx;
    u32 wb_line_addr;
    u32 mem_beat_idx;
    u32 cmo_user;
    u32 cmo_line_idx;
    bool cmo_hit;
    bool data_write_used;

    u64 *perf_hit;
    u64 *perf_miss;
    u64 *perf_bypass;
    u64 *perf_writeback;
    u64 *perf_bypass_rd_ost_full;
    u64 *perf_bypass_wr_ost_full;
    u64 *perf_miss_busy;
    u64 *perf_stg_ar_full[L2_PORT_NUM];
    u64 *perf_stg_aw_full[L2_PORT_NUM];
    u64 *perf_stg_w_full[L2_PORT_NUM];
} l2_t;

extern void l2_construct(l2_t *l2, const char *parent, const char *name,
    const l2_conf_t *conf);
extern void l2_reset(l2_t *l2);
extern void l2_clock(l2_t *l2);
extern void l2_free(l2_t *l2);

#endif
