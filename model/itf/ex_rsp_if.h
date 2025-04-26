#ifndef EX_RSP_IF_H
#define EX_RSP_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct ex_rsp_if {
    u32 pc;
    bool taken;
    bool pred_true;
    u32 target_pc;
} ex_rsp_if_t;

static inline void ex_rsp_if_to_str(const void *pkt, char *str)
{
    const ex_rsp_if_t *ex_rsp = (const ex_rsp_if_t *)pkt;
    sprintf(str, "%d %x\n", ex_rsp->taken, ex_rsp->target_pc);
}

#endif