#ifndef FL_REQ_IF_H
#define FL_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define FL_REQ_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &fl_req_if_to_str, &fl_req_if_reg_vcd_sig, sizeof(fl_req_if_t), depth)

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
