#ifndef AXI4_B_H
#define AXI4_B_H

#include <stdio.h>
#include "base/types.h"

typedef struct axi4_b_if {
    u8 id;
    u8 resp : 2;
} axi4_b_if_t;

static inline void axi4_b_if_to_str(const void *pkt, char *str)
{
    const axi4_b_if_t *axi4_b = (const axi4_b_if_t *)pkt;
    sprintf(str, "%02x %01x\n", axi4_b->id, axi4_b->resp);
}

#endif
