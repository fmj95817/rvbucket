#include "rom.h"
#include <stdlib.h>
#include <string.h>
#include "dbg.h"

void rom_construct(rom_t *rom, u32 size, const void *data, u32 data_size)
{
    rom->size = size;
    rom->data = malloc(size + 3);
    DBG_CHECK(rom->data);

    if (data != NULL) {
        DBG_CHECK(data_size <= size);
        memcpy(rom->data, data, data_size);
    }
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

static bool rom_read(rom_t *rom, u32 addr, u32 *data)
{
    if (addr >= rom->size) {
        return false;
    }

    *data = *((u32*)(rom->data + addr));
    return true;
}

void rom_bus_trans_handler(rom_t *rom, u32 base_addr, bus_trans_if_t *i)
{
    DBG_CHECK(i->req.addr >= base_addr);

    u32 addr = i->req.addr - base_addr;
    if (i->req.cmd == BUS_CMD_READ) {
        i->rsp.ok = rom_read(rom, addr, &i->rsp.data);
    } else {
        i->rsp.ok = false;
    }
}

