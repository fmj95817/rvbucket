#ifndef BIU_H
#define BIU_H

#include "types.h"
#include "mod_if.h"
#include "bus.h"

typedef struct biu {
    bus_if_t *bus_if;
} biu_t;

extern void biu_construct(biu_t *biu, bus_if_t *bus_if);
extern void biu_reset(biu_t *biu);
extern void biu_free(biu_t *biu);

extern void biu_ifetch_trans_handler(biu_t *biu, ifetch_if_t *i);
extern void biu_mem_rw_trans_handler(biu_t *biu, mem_rw_if_t *i);

#endif