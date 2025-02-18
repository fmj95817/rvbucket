#ifndef DBG_VCD_H
#define DBG_VCD_H

#include <stdio.h>
#include "base/types.h"

#define DBG_SIG_TOKEN_MAX 32
#define DBG_SIG_BIT_MAX 64
#define DBG_SIG_BYTE_MAX 8

typedef enum dbg_sig_type {
    DBG_SIG_TYPE_WIRE = 0,
    DBG_SIG_TYPE_REG,
} dbg_sig_type_t;

typedef struct dbg_sig {
    char token[DBG_SIG_TOKEN_MAX];
    u32 bits;
    u32 bytes;
    const void *cur;
    u8 old[DBG_SIG_BYTE_MAX];
    struct vcd_sig *nxt;
} dbg_sig_t;

typedef struct dbg_vcd {
    bool enable;

    const u64 *cycle;
    char clk_token[DBG_SIG_TOKEN_MAX];

    struct {
        dbg_sig_t *head;
        dbg_sig_t *tail;
    } sig_list;

    FILE *vcd_fp;
} dbg_vcd_t;

extern void dbg_vcd_scope_begin(const char *name);
extern void dbg_vcd_set_clk(const u64 *cycle);
extern void dbg_vcd_add_sig(const char *name, dbg_sig_type_t type, u32 bits, const void *ref);
extern void dbg_vcd_scope_end();
extern void dbg_vcd_reset();
extern void dbg_vcd_clock();

#endif