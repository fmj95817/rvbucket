#ifndef AXI4_AW_H
#define AXI4_AW_H

#include <stdio.h>
#include "base/types.h"

typedef struct axi4_aw_if {
    u8 id;
    u32 addr;
    u8 len;
    u8 size : 3;
    u8 burst : 2;
    bool lock;
    u8 cache : 4;
    u8 prot : 3;
    u8 qos : 4;
    u32 user;
} axi4_aw_if_t;

static inline void axi4_aw_if_to_str(const void *pkt, char *str)
{
    const axi4_aw_if_t *axi4_aw = (const axi4_aw_if_t *)pkt;
    sprintf(str, "%02x %08x %02x %01x %01x %01x %01x %01x %01x %08x\n", axi4_aw->id, axi4_aw->addr, axi4_aw->len, axi4_aw->size, axi4_aw->burst, axi4_aw->lock, axi4_aw->cache, axi4_aw->prot, axi4_aw->qos, axi4_aw->user);
}

#endif
