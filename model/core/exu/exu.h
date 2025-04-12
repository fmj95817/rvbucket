#ifndef EXU_H
#define EXU_H

#include "base/types.h"
#include "base/itf.h"
#include "core/mod_if.h"
#include "core/isa.h"

typedef struct exu {
    itf_t *fl_req_slv;
    itf_t *ex_req_slv;
    itf_t *ex_rsp_mst;
    itf_t *ldst_req_mst;
    itf_t *ldst_rsp_slv;

    bool ldst_req_pend;
    u32 ldst_opcode;
    u32 ld_rd;
    u32 ld_funct3;

    u32 gpr[32];
} exu_t;

extern void exu_construct(exu_t *exu);
extern void exu_reset(exu_t *exu);
extern void exu_clock(exu_t *exu);
extern void exu_free(exu_t *exu);

extern void ldst_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void alu_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void branch_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void misc_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);
extern void sys_ex_req_proc(exu_t *exu, const ex_req_if_t *ex_req);

extern void ldst_biu_rsp_proc(exu_t *exu, const ldst_rsp_if_t *ldst_rsp);

#endif