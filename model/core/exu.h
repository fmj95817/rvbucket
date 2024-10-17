#ifndef EXU_H
#define EXU_H

#include "types.h"
#include "rv32i.h"
#include "isa.h"

extern void inst_handler(rv32i_t *s, const rv32i_inst_t *i, u32 *pc_offset);

#endif