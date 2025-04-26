#ifndef FCH_RSP_IF_H
#define FCH_RSP_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct fch_rsp_if {
    u32 ir;
    bool ok;
} fch_rsp_if_t;

static inline void fch_rsp_if_to_str(const void *pkt, char *str)
{
    const fch_rsp_if_t *fch_rsp = (const fch_rsp_if_t *)pkt;
    sprintf(str, "%x %d\n", fch_rsp->ir, fch_rsp->ok);
}

#endif