#ifndef L1_FLUSH_IF_H
#define L1_FLUSH_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define L1_FLUSH_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(l1_flush_if_t), \
        .pkt2str = &l1_flush_if_to_str, \
        .reg_vcd = &l1_flush_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct l1_flush_if {
    u32 dummy;
} l1_flush_if_t;

static inline void l1_flush_if_to_str(const void *pkt, char *str)
{
    const l1_flush_if_t *flush = (const l1_flush_if_t *)pkt;
    sprintf(str, "%08x\n", flush->dummy);
}

static inline void l1_flush_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const l1_flush_if_t *flush = (const l1_flush_if_t *)pkt;
    dbg_vcd_add_sig("dummy", type, 32, &flush->dummy);
}

#endif
