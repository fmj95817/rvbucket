#ifndef BTI_REQ_IF_H
#define BTI_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define BTI_REQ_SIGNAL_IF_CONSTRUCT(module, itf, dis_dump, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(bti_req_if_t), \
        .pkt2str = &bti_req_if_to_str, \
        .reg_vcd = &bti_req_if_reg_vcd, \
        .force_disable_dump = dis_dump, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

#define BTI_REQ_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .mode = ITF_MODE_FIFO, \
        .pkt_size = sizeof(bti_req_if_t), \
        .pkt2str = &bti_req_if_to_str, \
        .reg_vcd = &bti_req_if_reg_vcd, \
        .force_disable_dump = false, \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef enum bti_req_cmd {
    BTI_REQ_CMD_READ = 0,
    BTI_REQ_CMD_WRITE = 1,
} bti_req_cmd_t;

typedef struct bti_req_if {
    u16 trans_id;
    bti_req_cmd_t cmd;
    u32 addr;
    u32 data;
    u8 strobe;
} bti_req_if_t;

static inline void bti_req_if_to_str(const void *pkt, char *str)
{
    const bti_req_if_t *bti_req = (const bti_req_if_t *)pkt;
    sprintf(str, "%04x %01x %08x %08x %02x\n", bti_req->trans_id, (u32)bti_req->cmd, bti_req->addr, bti_req->data, bti_req->strobe);
}

static inline void bti_req_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const bti_req_if_t *bti_req = (const bti_req_if_t *)pkt;
    dbg_vcd_add_sig("trans_id", type, 16, &bti_req->trans_id);
    dbg_vcd_add_sig("cmd", type, 1, &bti_req->cmd);
    dbg_vcd_add_sig("addr", type, 32, &bti_req->addr);
    dbg_vcd_add_sig("data", type, 32, &bti_req->data);
    dbg_vcd_add_sig("strobe", type, 8, &bti_req->strobe);
}

#endif
