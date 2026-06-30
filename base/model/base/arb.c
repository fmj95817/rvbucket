#include "arb.h"

void arb_construct(arb_t *arb, u32 req_num)
{
    arb->req_num = req_num;
    arb->req = malloc(sizeof(bool) * req_num);
}

void arb_reset(arb_t *arb)
{
    for (u32 i = 0; i < arb->req_num; i++) {
        arb->req[i] = false;
    }
    arb->grant_idx = -1;
}

void arb_clock(arb_t *arb)
{
    for (u32 i = 0; i < arb->req_num; i++) {
        s32 nxt = (arb->grant_idx + i + 1) % arb->req_num;
        if (arb->req[nxt]) {
            arb->grant_idx = nxt;
            return;
        }
    }
    arb->grant_idx = -1;
}

void arb_free(arb_t *arb)
{
    if (arb->req) {
        free(arb->req);
        arb->req = NULL;
    }
}