#ifndef APB_REQ_H
#define APB_REQ_H

#include <stdio.h>
#include "base/types.h"

typedef struct apb_req_if {
    u32 paddr;
    bool pwrite;
    u32 pwdata;
    u8 pstrb : 4;
} apb_req_if_t;

static inline void apb_req_if_to_str(const void *pkt, char *str)
{
    const apb_req_if_t *apb_req = (const apb_req_if_t *)pkt;
    sprintf(str, "%08x %01x %08x %01x\n", apb_req->paddr, apb_req->pwrite, apb_req->pwdata, apb_req->pstrb);
}

#endif
