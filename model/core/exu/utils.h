#ifndef EXU_UTILS_H
#define EXU_UTILS_H

#include "base/types.h"
#include "core/isa.h"
#include "exu.h"
#include "dbg/chk.h"

static inline u32 sign_ext(u32 data, u32 width)
{
    if (data & (1u << (width - 1))) {
        data |= ((~0u) << width);
    }
    return data;
}

static inline u32 u_imm_decode(const rv32i_inst_t *i)
{
    return i->u.imm_31_12 << 12;
}

static inline i32 j_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->j.imm_10_1 << 1) |
              (i->j.imm_11 << 11) |
              (i->j.imm_19_12 << 12) |
              (i->j.imm_20 << 20);

    i32 dst = { .u = sign_ext(src, 21) };
    return dst;
}

static inline i32 i_imm_decode(const rv32i_inst_t *i)
{
    i32 dst = { .u = sign_ext(i->i.imm_11_0, 12) };
    return dst;
}

static inline i32 b_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->b.imm_4_1 << 1) |
              (i->b.imm_10_5 << 5) |
              (i->b.imm_11 << 11) |
              (i->b.imm_12 << 12);

    i32 dst = { .u = sign_ext(src, 13) };
    return dst;
}

static inline i32 s_imm_decode(const rv32i_inst_t *i)
{
    u32 src = (i->s.imm_4_0) |
              (i->s.imm_11_5 << 5);

    i32 dst = { .u = sign_ext(src, 12) };
    return dst;
}

static inline u32 get_gpr(const exu_t *exu, u32 i)
{
    return exu->gpr[i];
}

static inline void set_gpr(exu_t *exu, u32 i, u32 val)
{
    if (i > 0) {
        exu->gpr[i] = val;
    }
}

static inline const char *gpr_name(u32 i)
{
    DBG_CHECK(i < 32);

    static const char *names[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };

    return names[i];
}
#endif