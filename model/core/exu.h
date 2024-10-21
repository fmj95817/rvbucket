#ifndef EXU_H
#define EXU_H

#include "types.h"
#include "mod_if.h"
#include "isa.h"
#include "lsu.h"
#include "dbg.h"

typedef struct exu {
    lsu_t *lsu;
    u32 gpr[32];
} exu_t;

extern void exu_construct(exu_t *exu, lsu_t *lsu);
extern void exu_reset(exu_t *exu);
extern void exu_free(exu_t *exu);

extern void exu_exec(exu_t *exu, iexec_if_t *i);

#endif