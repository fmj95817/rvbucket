#ifndef BPU_H
#define BPU_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/bpu_pred_req_if.h"
#include "itf/bpu_pred_rsp_if.h"
#include "itf/bpu_update_if.h"
#include "spec/core/hart.h"

typedef struct bpu_ras {
    u32 entry[BPU_RAS_SIZE];
    u32 sp;
    u32 count;
} bpu_ras_t;

typedef struct bpu {
    mod_t mod;
    itf_t *pred_req_slv;
    itf_t *pred_rsp_mst;
    itf_t *update_slv;
    const bpu_pred_req_if_t *pred_req_i;

    u32 access_seq;

    struct {
        bool vld;
        u8 counter;
    } cond_bht[BPU_COND_BHT_SIZE];

    struct {
        bool vld;
        u32 pc;
        u32 target_pc;
        u32 last_used;
    } jalr_btb[BPU_JALR_BTB_SIZE];

    bpu_ras_t ras;

    struct {
        u64 *branch;
        u64 *pred_true;
        u64 *cond_branch;
        u64 *cond_branch_pred_true;
        u64 *jal;
        u64 *jal_pred_true;
        u64 *jalr;
        u64 *jalr_pred_true;
        u64 *cond_bht_hit;
        u64 *ras_pred;
        u64 *jalr_ras_hit;
        u64 *jalr_btb_hit;
        u64 *jalr_btb_miss;
    } perf;
} bpu_t;

extern void bpu_construct(bpu_t *bpu, const char *parent, const char *name);
extern void bpu_reset(bpu_t *bpu);
extern void bpu_clock(bpu_t *bpu);
extern void bpu_free(bpu_t *bpu);

#endif
