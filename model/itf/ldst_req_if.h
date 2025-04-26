#ifndef LDST_REQ_IF_H
#define LDST_REQ_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct ldst_req_if {
    u32 addr;
    bool st;
    u32 data;
    u8 strobe;
} ldst_req_if_t;

static inline void ldst_req_if_to_str(const void *pkt, char *str)
{
    const ldst_req_if_t *ldst_req = (const ldst_req_if_t *)pkt;
    sprintf(str, "%x %x %d %x\n", ldst_req->addr, ldst_req->data, ldst_req->st, ldst_req->strobe);
}

#endif