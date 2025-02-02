#include "ram.h"
#include <stdlib.h>
#include <string.h>
#include "dbg.h"

void ram_construct(ram_t *ram, u32 size, u32 base_addr)
{
    ram->size = size;
    ram->data = malloc(size + 3);
    ram->base_addr = base_addr;
    DBG_CHECK(ram->data);
}

void ram_reset(ram_t *ram) {}

void ram_free(ram_t *ram)
{
    free(ram->data);
}

static bool ram_read(ram_t *ram, u32 addr, u32 *data)
{
    if (addr >= ram->size) {
        return false;
    }

    *data = *((u32*)(ram->data + addr));
    return true;
}

static bool ram_write(ram_t *ram, u32 addr, u32 data, u8 strobe)
{
    if (addr >= ram->size) {
        return false;
    }

    u8 *dst = (ram->data + addr);

    if (strobe & 0b0001) {
        dst[0] = (data & 0xff);
    }

    if (strobe & 0b0010) {
        dst[1] = ((data >> 8) & 0xff);
    }

    if (strobe & 0b0100) {
        dst[2] = ((data >> 16) & 0xff);
    }

    if (strobe & 0b1000) {
        dst[3] = ((data >> 24) & 0xff);
    }

    return true;
}

void ram_clock(ram_t *ram)
{
    if (itf_fifo_empty(ram->bti_req_slv)) {
        return;
    }

    if (itf_fifo_full(ram->bti_rsp_mst)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(ram->bti_req_slv, &bti_req);
    DBG_CHECK(bti_req.addr >= ram->base_addr);

    bti_rsp_if_t bti_rsp;
    bti_rsp.trans_id = bti_req.trans_id;
    u32 addr = bti_req.addr - ram->base_addr;

    if (bti_req.cmd == BTI_CMD_READ) {
        bti_rsp.ok = ram_read(ram, addr, &bti_rsp.data);
    } else if (bti_req.cmd == BTI_CMD_WRITE) {
        bti_rsp.ok = ram_write(ram, addr, bti_req.data, bti_req.strobe);
    } else {
        bti_rsp.ok = false;
    }

    itf_write(ram->bti_rsp_mst, &bti_rsp);
}
