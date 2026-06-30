#ifndef EXU_UTILS_H
#define EXU_UTILS_H

#include "base/types.h"
#include "spec/core/isa.h"
#include "exu.h"
#include "dbg/chk.h"

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
    DBG_CHECK(i < RV32G_GPR_NUM);

    static const char *names[RV32G_GPR_NUM] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };

    return names[i];
}
#endif
