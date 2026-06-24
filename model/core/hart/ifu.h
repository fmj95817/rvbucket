#ifndef IFU_H
#define IFU_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/ex_req_if.h"
#include "itf/ex_rsp_if.h"
#include "itf/fch_req_if.h"
#include "itf/fch_rsp_if.h"
#include "itf/fl_req_if.h"
#include "itf/trap_send_if.h"
#include "spec/core/hart.h"

#ifndef IFU_CTRLQ_SIZE
#define IFU_CTRLQ_SIZE 16
#endif

typedef enum ifu_fch_state {
    IFU_FCH_STATE_REQ = 0,
    IFU_FCH_STATE_PEND,
    IFU_FCH_STATE_RSP
} ifu_fch_state_t;

typedef enum ifu_redirect_state {
    IFU_REDIRECT_STATE_IDLE = 0,
    IFU_REDIRECT_STATE_BRANCH,
    IFU_REDIRECT_STATE_TRAP,
    IFU_REDIRECT_STATE_REDIRECT
} ifu_redirect_state_t;

typedef struct ifu {
    itf_t *fch_req_mst;
    itf_t *fch_rsp_slv;
    itf_t *ex_req_mst;
    itf_t *ex_rsp_slv;
    itf_t *fl_req_mst;
    itf_t *trap_send_slv;

    u32 reset_pc;

    struct {
        ifu_fch_state_t state;
        u32 pc;
        u32 ir;
    } fch;

    struct {
        bool vld;
        u32 pc;
        u32 ir;
        u32 pred_target_pc;
        bool pred_taken;
        bool is_ctrl;
    } issue;

    struct {
        ifu_redirect_state_t state;
        u32 pc;
    } redirect;

    struct {
        u32 head;
        u32 tail;
        u32 count;
        struct {
            bool vld;
            u32 pc;
        } entry[IFU_CTRLQ_SIZE];
    } ctrlq;

    struct {
        bool enable;
        u32 access_seq;
        struct {
            bool vld;
            u32 pc;
            u8 counter;
            u32 target_pc;
            u32 last_used;
        } bht[IFU_BHT_SIZE];
    } bpu;

    struct {
        u32 base;
        u32 size;
    } boot_rom_info;

    struct {
        u64 *branch;
        u64 *pred_true;
    } perf;
} ifu_t;

extern void ifu_construct(ifu_t *ifu, const char *name, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size);
extern void ifu_reset(ifu_t *ifu);
extern void ifu_clock(ifu_t *ifu);
extern void ifu_free(ifu_t *ifu);

#endif