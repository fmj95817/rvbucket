#ifndef AXI4_R_H
#define AXI4_R_H

#include <stdio.h>
#include "base/types.h"

typedef struct axi4_r_if {
    u8 id;
    u32 data;
    u8 resp : 2;
    bool last;
} axi4_r_if_t;

static inline void axi4_r_if_to_str(const void *pkt, char *str)
{
    const axi4_r_if_t *axi4_r = (const axi4_r_if_t *)pkt;
    sprintf(str, "%02x %08x %01x %01x\n", axi4_r->id, axi4_r->data, axi4_r->resp, axi4_r->last);
}

#endif
