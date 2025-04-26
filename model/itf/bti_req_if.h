#ifndef BTI_REQ_IF_H
#define BTI_REQ_IF_H

#include <stdio.h>
#include "base/types.h"

typedef enum bti_cmd {
    BTI_CMD_READ = 0,
    BTI_CMD_WRITE = 1
} bti_cmd_t;

typedef struct bti_req_if {
    u32 trans_id;
    bti_cmd_t cmd;
    u32 addr;
    u32 data;
    u8 strobe;
} bti_req_if_t;

static inline void bti_req_if_to_str(const void *pkt, char *str)
{
    const bti_req_if_t *bti_req = (const bti_req_if_t *)pkt;
    sprintf(str, "%u %d %x %x %x\n", bti_req->trans_id,
        bti_req->cmd, bti_req->addr, bti_req->data, bti_req->strobe);
}

#endif