#ifndef IFU_H
#define IFU_H

#include "types.h"
#include "lsu.h"

typedef struct ifu {
    lsu_t *lsu;
    u32 pc;
    u32 reset_pc;
} ifu_t;

extern void ifu_construct(ifu_t *ifu, lsu_t *lsu, u32 reset_pc);
extern void ifu_reset(ifu_t *ifu);
extern void ifu_free(ifu_t *ifu);

extern void ifu_exec(ifu_t *ifu, ifu2exu_trans_t *t);
extern void ifu_update(ifu_t *ifu, const ifu2exu_trans_t *t);

#endif