#include "ram.h"
#include <stdlib.h>
#include <string.h>
#include "dbg.h"

void ram_construct(ram_t *ram, u32 size)
{
    ram->size = size;
    ram->data = malloc(size + 3);
    DBG_CHECK(ram->data);
}

void ram_reset(ram_t *ram) {}

void ram_free(ram_t *ram)
{
    free(ram->data);
}

void ram_load(ram_t *ram, const void *data, u32 addr, u32 size)
{
    DBG_CHECK(addr < ram->size);
    DBG_CHECK(addr + size <= ram->size);
    memcpy(ram->data + addr, data, size);
}

bool ram_read(ram_t *ram, u32 addr, u32 *data)
{
    if (addr >= ram->size) {
        return false;
    }

    *data = *((u32*)(ram->data + addr));
    return true;
}

bool ram_write(ram_t *ram, u32 addr, u32 data, u8 strobe)
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

bus_rsp_t ram_bus_req_handler(ram_t *ram, u32 base_addr, const bus_req_t *req)
{
    DBG_CHECK(req->addr >= base_addr);

    u32 addr = req->addr - base_addr;
    bus_rsp_t rsp;

    if (req->cmd == BUS_CMD_READ) {
        rsp.ok = ram_read(ram, addr, &rsp.data);
    } else if (req->cmd == BUS_CMD_WRITE) {
        rsp.ok = ram_write(ram, addr, req->data, req->strobe);
    } else {
        rsp.ok = false;
    }

    return rsp;
}