#ifndef FL_REQ_IF_H
#define FL_REQ_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct fl_req_if {
    u32 dummy;
} fl_req_if_t;

static inline void fl_req_if_to_str(const void *pkt, char *str)
{
    const fl_req_if_t *fl_req = (const fl_req_if_t *)pkt;
    sprintf(str, "%u\n", fl_req->dummy);
}

#endif