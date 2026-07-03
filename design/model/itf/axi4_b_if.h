#ifndef AXI4_B_IF_H
#define AXI4_B_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define AXI4_B_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(axi4_b_if_t), \
        .pkt2str = &axi4_b_if_to_str, \
        .reg_vcd = &axi4_b_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define AXI4_B_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(axi4_b_if_t), \
        .pkt2str = &axi4_b_if_to_str, \
        .reg_vcd = &axi4_b_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum axi4_b_resp {
    AXI4_B_RESP_OKAY = 0,
    AXI4_B_RESP_EXOKAY = 1,
    AXI4_B_RESP_SLVERR = 2,
    AXI4_B_RESP_DECERR = 3
} axi4_b_resp_t;

typedef struct axi4_b_if {
    u8 id;
    axi4_b_resp_t resp;
} axi4_b_if_t;

static inline void axi4_b_if_to_str(const void *pkt, char *str)
{
    const axi4_b_if_t *axi4_b = (const axi4_b_if_t *)pkt;
    sprintf(str, "%02x %01x\n", axi4_b->id, (u32)axi4_b->resp);
}

static inline void axi4_b_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const axi4_b_if_t *axi4_b = (const axi4_b_if_t *)pkt;
    dbg_vcd_add_sig("id", type, 8, &axi4_b->id);
    dbg_vcd_add_sig("resp", type, 2, &axi4_b->resp);
}

#endif
