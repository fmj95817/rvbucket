#ifndef SPEC_CORE_CACHE_H
#define SPEC_CORE_CACHE_H

#include "base/types.h"

#define CACHE_CBO_BLOCK_SIZE 64u

typedef enum axi4_user_cmo_op {
    AXI4_USER_CMO_OP_NONE  = 0,
    AXI4_USER_CMO_OP_INVAL = 1,
    AXI4_USER_CMO_OP_CLEAN = 2,
    AXI4_USER_CMO_OP_FLUSH = 3
} axi4_user_cmo_op_t;

#define AXI4_USER_CMO_TAG 0x30u

typedef struct __attribute__((packed)) axi4_user_cmo {
    u32 op   : 2;
    u32 tag  : 6;
    u32 rsvd : 24;
} axi4_user_cmo_t;

typedef union __attribute__((packed)) axi4_user {
    axi4_user_cmo_t cmo;
    u32 raw;
} axi4_user_t;

_Static_assert(sizeof(axi4_user_t) == sizeof(u32),
    "AXI4 user field must stay 32 bits");

static inline u32 axi4_user_make_cmo(axi4_user_cmo_op_t op)
{
    axi4_user_t user = {
        .cmo = {
            .op = op,
            .tag = AXI4_USER_CMO_TAG,
            .rsvd = 0
        }
    };
    return user.raw;
}

#define AXI4_USER_CMO_INVAL axi4_user_make_cmo(AXI4_USER_CMO_OP_INVAL)
#define AXI4_USER_CMO_CLEAN axi4_user_make_cmo(AXI4_USER_CMO_OP_CLEAN)
#define AXI4_USER_CMO_FLUSH axi4_user_make_cmo(AXI4_USER_CMO_OP_FLUSH)

static inline axi4_user_cmo_op_t axi4_user_cmo_op(u32 raw)
{
    axi4_user_t user = {.raw = raw};
    return user.cmo.op;
}

static inline bool axi4_user_is_cmo(u32 user)
{
    axi4_user_t parsed = {.raw = user};
    return parsed.cmo.tag == AXI4_USER_CMO_TAG && parsed.cmo.rsvd == 0;
}

static inline bool axi4_user_cmo_invalidates(u32 user)
{
    axi4_user_cmo_op_t op = axi4_user_cmo_op(user);
    return op == AXI4_USER_CMO_OP_INVAL || op == AXI4_USER_CMO_OP_FLUSH;
}

static inline bool axi4_user_cmo_cleans(u32 user)
{
    axi4_user_cmo_op_t op = axi4_user_cmo_op(user);
    return op == AXI4_USER_CMO_OP_CLEAN || op == AXI4_USER_CMO_OP_FLUSH;
}

#endif
