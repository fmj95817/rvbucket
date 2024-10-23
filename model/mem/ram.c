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

void ram_bus_trans_handler(ram_t *ram, u32 base_addr, bus_trans_if_t *i)
{
    DBG_CHECK(i->req.addr >= base_addr);

    u32 addr = i->req.addr - base_addr;
    if (i->req.cmd == BUS_CMD_READ) {
        i->rsp.ok = ram_read(ram, addr, &i->rsp.data);
    } else if (i->req.cmd == BUS_CMD_WRITE) {
        i->rsp.ok = ram_write(ram, addr, i->req.data, i->req.strobe);
    } else {
        i->rsp.ok = false;
    }
}