#ifndef LSU_H
#define LSU_H

#include "types.h"
#include "trans.h"
#include "bus.h"

typedef struct lsu {
    bus_if_t *bus_if;
} lsu_t;

extern void lsu_construct(lsu_t *lsu, bus_if_t *bus_if);
extern void lsu_reset(lsu_t *lsu);
extern void lsu_free(lsu_t *lsu);

extern void lsu_exu_trans_handler(lsu_t *lsu, exu2lsu_trans_t *t);
extern void lsu_ifu_trans_handler(lsu_t *lsu, ifu2lsu_trans_t *t);

#endif