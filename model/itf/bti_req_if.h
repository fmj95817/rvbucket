#ifndef BTI_REQ_IF_H
#define BTI_REQ_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define BTI_REQ_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &bti_req_if_to_str, &bti_req_if_reg_vcd_sig, sizeof(bti_req_if_t), depth)

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

static inline void bti_req_if_reg_vcd_sig(const void *pkt)
{
    const bti_req_if_t *bti_req = (const bti_req_if_t *)pkt;
    dbg_vcd_add_sig("trans_id", DBG_SIG_TYPE_REG, 16, &bti_req->trans_id);
    dbg_vcd_add_sig("cmd", DBG_SIG_TYPE_REG, 1, &bti_req->cmd);
    dbg_vcd_add_sig("addr", DBG_SIG_TYPE_REG, 32, &bti_req->addr);
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &bti_req->data);
    dbg_vcd_add_sig("strobe", DBG_SIG_TYPE_REG, 8, &bti_req->strobe);
}

#endif
