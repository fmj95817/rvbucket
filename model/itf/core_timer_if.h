#ifndef CORE_TIMER_IF_H
#define CORE_TIMER_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define CORE_TIMER_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(core_timer_if_t), \
        .pkt2str = &core_timer_if_to_str, \
        .reg_vcd = &core_timer_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define CORE_TIMER_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(core_timer_if_t), \
        .pkt2str = &core_timer_if_to_str, \
        .reg_vcd = &core_timer_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct core_timer_if {
    u64 time;
} core_timer_if_t;

static inline void core_timer_if_to_str(const void *pkt, char *str)
{
    const core_timer_if_t *core_timer = (const core_timer_if_t *)pkt;
    sprintf(str, "%016lx\n", core_timer->time);
}

static inline void core_timer_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const core_timer_if_t *core_timer = (const core_timer_if_t *)pkt;
    dbg_vcd_add_sig("time", type, 64, &core_timer->time);
}

#endif
