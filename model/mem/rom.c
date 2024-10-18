#include "rom.h"
#include <stdlib.h>
#include <string.h>
#include "dbg.h"

void rom_construct(rom_t *rom, u32 size)
{
    rom->size = size;
    rom->data = malloc(size + 3);
    DBG_CHECK(rom->data);
}

void rom_reset(rom_t *rom) {}

void rom_free(rom_t *rom)
{
    free(rom->data);
}

void rom_burn(rom_t *rom, const void *data, u32 addr, u32 size)
{
    DBG_CHECK(addr < rom->size);
    DBG_CHECK(addr + size <= rom->size);
    memcpy(rom->data + addr, data, size);
}

bool rom_read(rom_t *rom, u32 addr, u32 *data)
{
    if (addr >= rom->size) {
        return false;
    }

    *data = *((u32*)(rom->data + addr));
    return true;
}

bus_rsp_t rom_bus_req_handler(rom_t *rom, u32 base_addr, const bus_req_t *req)
{
    DBG_CHECK(req->addr >= base_addr);

    u32 addr = req->addr - base_addr;
    bus_rsp_t rsp;

    if (req->cmd == BUS_CMD_READ) {
        rsp.ok = rom_read(rom, addr, &rsp.data);
    } else {
        rsp.ok = false;
    }

    return rsp;
}

