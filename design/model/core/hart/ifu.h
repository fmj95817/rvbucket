#ifndef IFU_H
#define IFU_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "base/fifo.h"
#include "base/ost.h"
#include "itf/ex_req_if.h"
#include "itf/ex_rsp_if.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/fl_req_if.h"
#include "itf/bpu_pred_req_if.h"
#include "itf/bpu_pred_rsp_if.h"
#include "itf/bpu_update_if.h"
#include "itf/hart_expt_if.h"
#include "itf/l1_flush_if.h"
#include "itf/trap_send_if.h"
#include "itf/tlb_flush_if.h"
#include "itf/exu_state_if.h"
#include "spec/core/hart.h"

typedef struct ifu_conf {
    u32 reset_pc;
    u32 boot_rom_base;
    u32 boot_rom_size;
    u32 ctrlq_depth;
    u32 fch_ost_depth;
} ifu_conf_t;

typedef struct ifu_ctrlq_entry {
    bool vld;
    u32 pc;
    u32 ir;
    bool pred_taken;
    u32 pred_pc;
    bool pred_cond_bht_hit;
    bool pred_jalr_ras_hit;
    bool pred_jalr_btb_hit;
    bool pred_jalr_btb_miss;
} ifu_ctrlq_entry_t;

typedef struct ifu_fch_ost_ctx {
    u32 pc;
    u32 epoch;
} ifu_fch_ost_ctx_t;

typedef struct ifu_fch_rspq_entry {
    u32 pc;
    u32 ir;
    bool ok;
    bool expt;
    u32 cause;
    u8 priv;
    u32 tval;
} ifu_fch_rspq_entry_t;

typedef enum ifu_fch_state {
    IFU_FCH_STATE_REQ = 0,
    IFU_FCH_STATE_PEND,
    IFU_FCH_STATE_RSP,
    IFU_FCH_STATE_FAULT
} ifu_fch_state_t;

typedef enum ifu_redirect_state {
    IFU_REDIRECT_STATE_IDLE = 0,
    IFU_REDIRECT_STATE_BRANCH,
    IFU_REDIRECT_STATE_TRAP,
    IFU_REDIRECT_STATE_REDIRECT
} ifu_redirect_state_t;

typedef struct ifu {
    mod_t mod;
    itf_t *fch_req_mst;
    itf_t *fch_rsp_slv;
    itf_t *ex_req_mst;
    itf_t *ex_rsp_slv;
    itf_t *fl_req_mst;
    itf_t *bpu_pred_req_mst;
    itf_t *bpu_pred_rsp_slv;
    itf_t *bpu_update_mst;
    itf_t *fch_expt_mst;
    itf_t *trap_send_slv;
    itf_t *tlb_flush_slv;
    itf_t *l1i_flush_slv;
    itf_t *exu_state_in;
    const exu_state_if_t *exu_state_i;

    u32 reset_pc;
    u32 ctrlq_depth;
    u32 fch_ost_depth;

    struct {
        ifu_fch_state_t state;
        u32 pc;
        u32 ir;
        u32 epoch;
    } fch;
    bool back_blocked;

    ostq_t fch_ost;
    fifo_t fch_rspq;

    struct {
        bool vld;
        u32 pc;
        u32 ir;
        u32 pred_target_pc;
        bool pred_taken;
        bool pred_vld;
        bool pred_redirected;
        bool is_ctrl;
        bool pred_cond_bht_hit;
        bool pred_jalr_ras_hit;
        bool pred_jalr_btb_hit;
        bool pred_jalr_btb_miss;
    } issue;

    struct {
        ifu_redirect_state_t state;
        u32 pc;
    } redirect;

    fifo_t ctrlq;

    struct {
        u32 base;
        u32 size;
    } boot_rom_info;

    struct {
        u64 *fch_rsp_inst;
        u64 *fch_ost_full;
    } perf;
} ifu_t;

extern void ifu_construct(ifu_t *ifu, const char *parent, const char *name,
    const ifu_conf_t *conf);
extern void ifu_reset(ifu_t *ifu);
extern void ifu_clock(ifu_t *ifu);
extern void ifu_free(ifu_t *ifu);

#endif
