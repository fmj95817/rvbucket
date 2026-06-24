#ifndef TRAP_SEND_IF_H
#define TRAP_SEND_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define TRAP_SEND_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(trap_send_if_t), \
        .pkt2str = &trap_send_if_to_str, \
        .reg_vcd = &trap_send_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define TRAP_SEND_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(trap_send_if_t), \
        .pkt2str = &trap_send_if_to_str, \
        .reg_vcd = &trap_send_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct trap_send_if {
    u32 target_pc;
} trap_send_if_t;

static inline void trap_send_if_to_str(const void *pkt, char *str)
{
    const trap_send_if_t *trap_send = (const trap_send_if_t *)pkt;
    sprintf(str, "%08x\n", trap_send->target_pc);
}

static inline void trap_send_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const trap_send_if_t *trap_send = (const trap_send_if_t *)pkt;
    dbg_vcd_add_sig("target_pc", type, 32, &trap_send->target_pc);
}

#endif
