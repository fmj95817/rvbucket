#ifndef EX_REQ_IF_H
#define EX_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "core/isa.h"

typedef struct ex_req_if {
    rv32i_inst_t inst;
    u32 pc;
    bool pred_taken;
    u32 pred_pc;
    bool is_boot_code;
} ex_req_if_t;

static inline void ex_req_if_to_str(const void *pkt, char *str)
{
    const ex_req_if_t *ex_req = (const ex_req_if_t *)pkt;
    sprintf(str, "%x %x %d\n", ex_req->inst.raw, ex_req->pc, ex_req->is_boot_code);
}

#endif