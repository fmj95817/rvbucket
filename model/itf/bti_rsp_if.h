#ifndef BTI_RSP_IF_H
#define BTI_RSP_IF_H

#include <stdio.h>
#include "base/types.h"

typedef struct bti_rsp_if {
    u32 trans_id;
    u32 data;
    bool ok;
} bti_rsp_if_t;

static inline void bti_rsp_if_to_str(const void *pkt, char *str)
{
    const bti_rsp_if_t *bti_rsp = (const bti_rsp_if_t *)pkt;
    sprintf(str, "%u %x %d\n", bti_rsp->trans_id, bti_rsp->data, bti_rsp->ok);
}

#endif