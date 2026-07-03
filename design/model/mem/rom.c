#include "rom.h"
#include <stdlib.h>
#include <string.h>
#include "dbg/chk.h"
#include "dbg/vcd.h"

void rom_construct(rom_t *rom, const char *name, rom_mode_t mode,
    u32 size, const void *data, u32 data_size, u32 base_addr)
{
    DBG_VCD_MODULE_SCOPE(name);

    rom->mode = mode;
    rom->size = size;
    rom->data = malloc(size + 3);
    DBG_CHECK(rom->data);

    if (data != NULL) {
        DBG_CHECK(data_size <= size);
        memcpy(rom->data, data, data_size);
    }

    rom->base_addr = base_addr;
}

void rom_reset(rom_t *rom)
{
    rom->rd_active = false;
}

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

static bool rom_read(rom_t *rom, u32 addr, u32 byte_num, u32 *data)
{
    if (byte_num > sizeof(*data) || addr >= rom->size || byte_num > rom->size - addr) {
        return false;
    }
    *data = 0;
    memcpy(data, rom->data + addr, byte_num);
    return true;
}

static u32 axi_burst_next_addr(u32 addr, u8 burst_type, u8 burst_size)
{
    u32 byte_step = (u32)1 << burst_size;
    if (burst_type == AXI4_AR_BURST_FIXED) {
        return addr;
    } else {
        return addr + byte_step;
    }
}

static void rom_proc_axi_ar(rom_t *rom)
{
    if (rom->rd_active) {
        return;
    }
    if (itf_fifo_empty(rom->axi4_ar_slv)) {
        return;
    }

    axi4_ar_if_t ar;
    itf_read(rom->axi4_ar_slv, &ar);

    DBG_CHECK(ar.addr >= rom->base_addr);
    rom->rd_active = true;
    rom->rd_burst_addr = ar.addr - rom->base_addr;
    rom->rd_burst_id = ar.id;
    rom->rd_burst_len = ar.len;
    rom->rd_burst_size = ar.size;
    rom->rd_burst_type = ar.burst;
    rom->rd_burst_cnt = 0;
}

static void rom_proc_axi_r(rom_t *rom)
{
    if (!rom->rd_active) {
        return;
    }
    if (itf_fifo_full(rom->axi4_r_mst)) {
        return;
    }

    u32 addr = rom->rd_burst_addr;
    u32 data = 0;
    u32 byte_num = 1u << rom->rd_burst_size;
    bool ok = rom_read(rom, addr, byte_num, &data);
    axi4_r_resp_t resp = ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR;

    bool last = (rom->rd_burst_cnt == rom->rd_burst_len);

    axi4_r_if_t r = {
        .id = rom->rd_burst_id,
        .data = data,
        .resp = resp,
        .last = last
    };
    itf_write(rom->axi4_r_mst, &r);

    if (last) {
        rom->rd_active = false;
    } else {
        rom->rd_burst_addr = axi_burst_next_addr(addr, rom->rd_burst_type,
                                                  rom->rd_burst_size);
        rom->rd_burst_cnt++;
    }
}

static void rom_proc_axi(rom_t *rom)
{
    rom_proc_axi_r(rom);
    rom_proc_axi_ar(rom);
}

static void rom_proc_bti(rom_t *rom)
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

    bti_rsp_if_t bti_rsp = {
        .trans_id = bti_req.trans_id,
        .data = 0,
        .ok = false
    };
    u32 addr = bti_req.addr - rom->base_addr;
    u32 byte_num = 1u << bti_req.size;

    if (bti_req.size > BTI_REQ_SIZE_B4) {
        bti_rsp.data = 0;
        bti_rsp.ok = false;
    } else if (bti_req.cmd == BTI_REQ_CMD_READ) {
        bti_rsp.ok = rom_read(rom, addr, byte_num, &bti_rsp.data);
    } else {
        bti_rsp.data = 0;
        bti_rsp.ok = false;
    }

    itf_write(rom->bti_rsp_mst, &bti_rsp);
}

void rom_clock(rom_t *rom)
{
    if (rom->mode == ROM_MODE_AXI) {
        rom_proc_axi(rom);
    } else {
        rom_proc_bti(rom);
    }
}
