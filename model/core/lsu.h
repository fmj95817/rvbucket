#ifndef LSU_H
#define LSU_H

#include "types.h"
#include "trans.h"
#include "biu.h"

typedef struct lsu {
    biu_t *biu;
} lsu_t;

extern void lsu_construct(lsu_t *lsu, biu_t *biu);
extern void lsu_reset(lsu_t *lsu);
extern void lsu_free(lsu_t *lsu);

extern void lsu_exu_trans_handler(lsu_t *lsu, exu2lsu_trans_t *t);

#endif