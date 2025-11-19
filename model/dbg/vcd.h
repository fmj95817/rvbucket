#ifndef DBG_VCD_H
#define DBG_VCD_H

#include "base/types.h"

#define DBG_VCD_MODULE_SCOPE(name) \
    __attribute__((cleanup(dbg_vcd_scope_cleanup))) \
    void *_dummy_ptr_ = NULL; \
    dbg_vcd_scope_begin("module", name)

typedef enum dbg_sig_type {
    DBG_SIG_TYPE_WIRE = 0,
    DBG_SIG_TYPE_REG,
} dbg_sig_type_t;

extern void dbg_vcd_scope_begin(const char *type, const char *name);
extern void dbg_vcd_set_clk(const u64 *cycle);
extern void dbg_vcd_add_sig(const char *name, dbg_sig_type_t type, u32 bits, const void *ref);
extern void dbg_vcd_scope_end();
extern void dbg_vcd_reset();
extern void dbg_vcd_clock();

static inline void dbg_vcd_scope_cleanup(void *p)
{
    (void)p;
    dbg_vcd_scope_end();
}

#endif