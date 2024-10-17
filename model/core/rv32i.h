#ifndef RV32I_H
#define RV32I_H

#include "types.h"
#include "bus.h"

typedef struct rv32i {
    u32 pc;
    u32 gpr[32];

    bus_if_t *bus_if;
    u32 reset_pc;
} rv32i_t;

extern void rv32i_construct(rv32i_t *s, bus_if_t *bus_if, u32 reset_pc);
extern void rv32i_reset(rv32i_t *s);
extern void rv32i_free(rv32i_t *s);

extern void rv32i_exec(rv32i_t *s);

#endif