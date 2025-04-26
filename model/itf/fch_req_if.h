#ifndef FCH_REQ_IF_H
#define FCH_REQ_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct fch_req_if {
    u32 pc;
} fch_req_if_t;

static inline void fch_req_if_to_str(const void *pkt, char *str)
{
    const fch_req_if_t *fch_req = (const fch_req_if_t *)pkt;
    sprintf(str, "%x\n", fch_req->pc);
}

#endif