#ifndef BIU_H
#define BIU_H

#include "types.h"
#include "trans.h"
#include "bus.h"


typedef struct biu {
    bus_if_t *bus_if;
} biu_t;

extern void biu_construct(biu_t *biu, bus_if_t *bus_if);
extern void biu_reset(biu_t *biu);
extern void biu_free(biu_t *biu);

extern void biu_ifu_trans_handler(biu_t *biu, ifu2biu_trans_t *t);
extern void biu_lsu_trans_handler(biu_t *biu, lsu2biu_trans_t *t);

#endif