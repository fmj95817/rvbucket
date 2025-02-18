#ifndef MOD_IF_H
#define MOD_IF_H

#include "base/types.h"
#include "isa.h"

typedef struct ex_req_if {
    rv32i_inst_t inst;
    u32 pc;
    bool pred_taken;
    u32 pred_pc;
    bool is_boot_code;
} ex_req_if_t;

typedef struct ex_rsp_if {
    u32 pc;
    bool taken;
    bool pred_true;
    u32 target_pc;
} ex_rsp_if_t;

typedef struct fch_req_if {
    u32 pc;
} fch_req_if_t;

typedef struct fch_rsp_if {
    u32 ir;
    bool ok;
} fch_rsp_if_t;

typedef struct fl_req_if {
    u32 dummy;
} fl_req_if_t;

typedef struct fl_rsp_if {
    u32 dummy;
} fl_rsp_if_t;

typedef struct ldst_req_if {
    u32 addr;
    bool st;
    u32 data;
    u8 strobe;
} ldst_req_if_t;

typedef struct ldst_rsp_if {
    u32 data;
    bool ok;
} ldst_rsp_if_t;

static inline void ex_req_if_to_str(const void *pkt, char *str)
{
    const ex_req_if_t *ex_req = (const ex_req_if_t *)pkt;
    sprintf(str, "%x %x %d\n", ex_req->inst.raw, ex_req->pc, ex_req->is_boot_code);
}

static inline void ex_rsp_if_to_str(const void *pkt, char *str)
{
    const ex_rsp_if_t *ex_rsp = (const ex_rsp_if_t *)pkt;
    sprintf(str, "%d %x\n", ex_rsp->taken, ex_rsp->target_pc);
}

static inline void fch_req_if_to_str(const void *pkt, char *str)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    sprintf(str, "%x\n", fch_req->pc);
}

static inline void fch_rsp_if_to_str(const void *pkt, char *str)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    sprintf(str, "%x %d\n", fch_rsp->ir, fch_rsp->ok);
}

static inline void fl_req_if_to_str(const void *pkt, char *str)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    sprintf(str, "%u\n", fl_req->dummy);
}

static inline void fl_rsp_if_to_str(const void *pkt, char *str)
{
    const fl_rsp_if_t *fl_rsp = (const fl_rsp_if_t *)pkt;
    sprintf(str, "%u\n", fl_rsp->dummy);
}

static inline void ldst_req_if_to_str(const void *pkt, char *str)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    sprintf(str, "%x %x %d %x\n", ldst_req->addr, ldst_req->data, ldst_req->st, ldst_req->strobe);
}

static inline void ldst_rsp_if_to_str(const void *pkt, char *str)
{
    const ldst_rsp_if_t *ldst_rsp = (const ldst_rsp_if_t *)pkt;
    sprintf(str, "%x %d\n", ldst_rsp->data, ldst_rsp->ok);
}

#endif