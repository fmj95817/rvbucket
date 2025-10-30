#ifndef AXI4_W_H
#define AXI4_W_H

#include <stdio.h>
#include "base/types.h"

typedef struct axi4_w_if {
    u32 data;
    u8 strb : 4;
    bool last;
} axi4_w_if_t;

static inline void axi4_w_if_to_str(const void *pkt, char *str)
{
    const axi4_w_if_t *axi4_w = (const axi4_w_if_t *)pkt;
    sprintf(str, "%08x %01x %01x\n", axi4_w->data, axi4_w->strb, axi4_w->last);
}

#endif
