#ifndef FCH_REQ_H
#define FCH_REQ_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

typedef struct fch_req_if {
    u32 pc;
} fch_req_if_t;

static inline void fch_req_if_to_str(const void *pkt, char *str)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    sprintf(str, "%08x\n", fch_req->pc);
}

static inline void fch_req_if_reg_vcd_sig(const void *pkt)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    dbg_vcd_add_sig("pc", DBG_SIG_TYPE_REG, 32, &fch_req->pc);
}

#endif
