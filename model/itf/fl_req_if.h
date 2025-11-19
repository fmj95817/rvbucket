#ifndef FL_REQ_H
#define FL_REQ_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

typedef struct fl_req_if {
    u32 dummy;
} fl_req_if_t;

static inline void fl_req_if_to_str(const void *pkt, char *str)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    sprintf(str, "%08x\n", fl_req->dummy);
}

static inline void fl_req_if_reg_vcd_sig(const void *pkt)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    dbg_vcd_add_sig("dummy", DBG_SIG_TYPE_REG, 32, &fl_req->dummy);
}

#endif
