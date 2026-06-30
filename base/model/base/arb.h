#ifndef ARB_H
#define ARB_H

#include "types.h"

typedef struct arb {
    u32 req_num;
    bool *req;
    s32 grant_idx;
} arb_t;

extern void arb_construct(arb_t *arb, u32 req_num);
extern void arb_reset(arb_t *arb);
extern void arb_clock(arb_t *arb);
extern void arb_free(arb_t *arb);

#endif