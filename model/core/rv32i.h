#ifndef RV32I_H
#define RV32I_H

#include "types.h"
#include "ifu.h"
#include "exu.h"
#include "lsu.h"
#include "dbg.h"

typedef struct rv32i {
    ifu_t ifu;
    exu_t exu;
    lsu_t lsu;
    log_sys_t *log_sys;
} rv32i_t;

extern void rv32i_construct(rv32i_t *s, bus_if_t *bus_if, u32 reset_pc, log_sys_t *log_sys);
extern void rv32i_reset(rv32i_t *s);
extern void rv32i_free(rv32i_t *s);

extern void rv32i_exec(rv32i_t *s);

#endif