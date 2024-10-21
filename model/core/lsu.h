#ifndef LSU_H
#define LSU_H

#include "types.h"
#include "mod_if.h"
#include "biu.h"

typedef struct lsu {
    biu_t *biu;
} lsu_t;

extern void lsu_construct(lsu_t *lsu, biu_t *biu);
extern void lsu_reset(lsu_t *lsu);
extern void lsu_free(lsu_t *lsu);

extern void lsu_ldst_trans_handler(lsu_t *lsu, ldst_if_t *i);

#endif