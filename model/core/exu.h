#ifndef EXU_H
#define EXU_H

#include "types.h"
#include "trans.h"
#include "isa.h"
#include "lsu.h"
#include "dbg.h"

typedef struct exu {
    lsu_t *lsu;
    u32 gpr[32];
    log_sys_t *log_sys;
} exu_t;

extern void exu_construct(exu_t *exu, lsu_t *lsu, log_sys_t *log_sys);
extern void exu_reset(exu_t *exu);
extern void exu_free(exu_t *exu);

extern void exu_exec(exu_t *exu, ifu2exu_trans_t *t);
extern void exu_dump(exu_t *exu);

#endif