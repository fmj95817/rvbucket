#ifndef FL_REQ_IF_H
#define FL_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define FL_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &fl_req_if_to_str, \
        .reg_vcd = &fl_req_if_reg_vcd, \
        .pkt_size = sizeof(fl_req_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct fl_req_if {
    u32 dummy;
} fl_req_if_t;

static inline void fl_req_if_to_str(const void *pkt, char *str)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    sprintf(str, "%08x\n", fl_req->dummy);
}

static inline void fl_req_if_reg_vcd(const void *pkt)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    dbg_vcd_add_sig("dummy", DBG_SIG_TYPE_REG, 32, &fl_req->dummy);
}

#endif
