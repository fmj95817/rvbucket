#ifndef EX_REQ_IF_H
#define EX_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"
#include "spec/core/isa.h"

#define EX_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(ex_req_if_t), \
        .pkt2str = &ex_req_if_to_str, \
        .reg_vcd = &ex_req_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define EX_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(ex_req_if_t), \
        .pkt2str = &ex_req_if_to_str, \
        .reg_vcd = &ex_req_if_reg_vcd, \
        .force_disable_trace = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct ex_req_if {
    rv32g_inst_t inst;
    u32 pc;
    bool pred_taken;
    u32 pred_pc;
    bool is_boot_code;
} ex_req_if_t;

static inline void ex_req_if_to_str(const void *pkt, char *str)
{
    const ex_req_if_t *ex_req = (const ex_req_if_t *)pkt;
    sprintf(str, "%08x %08x %01x %08x %01x\n", ex_req->inst.raw, ex_req->pc, ex_req->pred_taken, ex_req->pred_pc, ex_req->is_boot_code);
}

static inline void ex_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const ex_req_if_t *ex_req = (const ex_req_if_t *)pkt;
    dbg_vcd_add_sig("inst", type, 32, &ex_req->inst.raw);
    dbg_vcd_add_sig("pc", type, 32, &ex_req->pc);
    dbg_vcd_add_sig("pred_taken", type, 1, &ex_req->pred_taken);
    dbg_vcd_add_sig("pred_pc", type, 32, &ex_req->pred_pc);
    dbg_vcd_add_sig("is_boot_code", type, 1, &ex_req->is_boot_code);
}

#endif
