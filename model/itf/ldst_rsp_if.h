#ifndef LDST_RSP_IF_H
#define LDST_RSP_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct ldst_rsp_if {
    u32 data;
    bool ok;
} ldst_rsp_if_t;

static inline void ldst_rsp_if_to_str(const void *pkt, char *str)
{
    const ldst_rsp_if_t *ldst_rsp = (const ldst_rsp_if_t *)pkt;
    sprintf(str, "%x %d\n", ldst_rsp->data, ldst_rsp->ok);
}

#endif