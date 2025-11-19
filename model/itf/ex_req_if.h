#ifndef EX_REQ_IF_H
#define EX_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"
#include "core/hart/isa.h"

#define EX_REQ_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &ex_req_if_to_str, &ex_req_if_reg_vcd_sig, sizeof(ex_req_if_t), depth)

typedef struct ex_req_if {
    rv32g_inst_t inst;
    u32 pc;
    bool pred_taken;
    u32 pred_pc;
    bool is_boot_code;
} ex_req_if_t;

static inline void ex_req_if_to_str(const void *pkt, char *str)
{
    const ex_req_if_t *ex_req = (const ex_req_if_t *)pkt;
    sprintf(str, "%08x %08x %01x %08x %01x\n", ex_req->inst.raw, ex_req->pc, ex_req->pred_taken, ex_req->pred_pc, ex_req->is_boot_code);
}

static inline void ex_req_if_reg_vcd_sig(const void *pkt)
{
    const ex_req_if_t *ex_req = (const ex_req_if_t *)pkt;
    dbg_vcd_add_sig("inst", DBG_SIG_TYPE_REG, 32, &ex_req->inst.raw);
    dbg_vcd_add_sig("pc", DBG_SIG_TYPE_REG, 32, &ex_req->pc);
    dbg_vcd_add_sig("pred_taken", DBG_SIG_TYPE_REG, 1, &ex_req->pred_taken);
    dbg_vcd_add_sig("pred_pc", DBG_SIG_TYPE_REG, 32, &ex_req->pred_pc);
    dbg_vcd_add_sig("is_boot_code", DBG_SIG_TYPE_REG, 1, &ex_req->is_boot_code);
}

#endif
