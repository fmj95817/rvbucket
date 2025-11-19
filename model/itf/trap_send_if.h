#ifndef TRAP_SEND_IF_H
#define TRAP_SEND_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define TRAP_SEND_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &trap_send_if_to_str, &trap_send_if_reg_vcd_sig, sizeof(trap_send_if_t), depth)

typedef struct trap_send_if {
    u32 target_pc;
} trap_send_if_t;

static inline void trap_send_if_to_str(const void *pkt, char *str)
{
    const trap_send_if_t *trap_send = (const trap_send_if_t *)pkt;
    sprintf(str, "%08x\n", trap_send->target_pc);
}

static inline void trap_send_if_reg_vcd_sig(const void *pkt)
{
    const trap_send_if_t *trap_send = (const trap_send_if_t *)pkt;
    dbg_vcd_add_sig("target_pc", DBG_SIG_TYPE_REG, 32, &trap_send->target_pc);
}

#endif
