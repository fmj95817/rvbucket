#include "rom.h"
#include <stdlib.h>
#include <string.h>
#include "dbg.h"

void rom_construct(rom_t *rom, u32 size, const void *data, u32 data_size, u32 base_addr)
{
    rom->size = size;
    rom->data = malloc(size + 3);
    DBG_CHECK(rom->data);

    if (data != NULL) {
        DBG_CHECK(data_size <= size);
        memcpy(rom->data, data, data_size);
    }

    rom->base_addr = base_addr;
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

void rom_clock(rom_t *rom)
{
    if (itf_fifo_empty(rom->bti_req_slv)) {
        return;
    }

    if (itf_fifo_full(rom->bti_rsp_mst)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(rom->bti_req_slv, &bti_req);
    DBG_CHECK(bti_req.addr >= rom->base_addr);

    bti_rsp_if_t bti_rsp;
    bti_rsp.trans_id = bti_req.trans_id;
    u32 addr = bti_req.addr - rom->base_addr;

    if (bti_req.cmd == BTI_CMD_READ) {
        bti_rsp.ok = rom_read(rom, addr, &bti_rsp.data);
    } else {
        bti_rsp.ok = false;
    }

    itf_write(rom->bti_rsp_mst, &bti_rsp);
}