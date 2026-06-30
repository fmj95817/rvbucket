#include "ram.h"
#include <stdlib.h>
#include <string.h>
#include "dbg/chk.h"
#include "dbg/vcd.h"

void ram_construct(ram_t *ram, const char *name, u32 port_num, ram_mode_t mode,
                   u32 size, u32 base_addr)
{
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(port_num > 0);
    DBG_CHECK(port_num <= RAM_MAX_PORT_NUM);

    ram->port_num = port_num;
    ram->mode = mode;
    ram->size = size;
    ram->data = malloc(size + 3);
    ram->base_addr = base_addr;
    DBG_CHECK(ram->data);
}

void ram_reset(ram_t *ram)
{
    ram->rd_active = false;
    ram->wr_active = false;
    ram->wr_b_pending = false;
}

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

static u32 axi_burst_next_addr(u32 addr, u8 burst_type, u8 burst_size)
{
    u32 byte_step = (u32)1 << burst_size;
    if (burst_type == AXI4_AR_BURST_FIXED) {
        return addr;
    } else {
        return addr + byte_step;
    }
}

static void ram_proc_axi_ar(ram_t *ram)
{
    if (ram->rd_active) {
        return;
    }
    if (itf_fifo_empty(ram->axi4_ar_slv)) {
        return;
    }

    axi4_ar_if_t ar;
    itf_read(ram->axi4_ar_slv, &ar);

    DBG_CHECK(ar.addr >= ram->base_addr);
    ram->rd_active = true;
    ram->rd_burst_addr = ar.addr - ram->base_addr;
    ram->rd_burst_id = ar.id;
    ram->rd_burst_len = ar.len;
    ram->rd_burst_size = ar.size;
    ram->rd_burst_type = ar.burst;
    ram->rd_burst_cnt = 0;
}

static void ram_proc_axi_r(ram_t *ram)
{
    if (!ram->rd_active) {
        return;
    }
    if (itf_fifo_full(ram->axi4_r_mst)) {
        return;
    }

    u32 addr = ram->rd_burst_addr;
    u32 data = 0;
    bool ok = ram_read(ram, addr, &data);
    axi4_r_resp_t resp = ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR;

    bool last = (ram->rd_burst_cnt == ram->rd_burst_len);

    axi4_r_if_t r = {
        .id = ram->rd_burst_id,
        .data = data,
        .resp = resp,
        .last = last
    };
    itf_write(ram->axi4_r_mst, &r);

    if (last) {
        ram->rd_active = false;
    } else {
        ram->rd_burst_addr = axi_burst_next_addr(addr, ram->rd_burst_type,
                                                  ram->rd_burst_size);
        ram->rd_burst_cnt++;
    }
}

static void ram_proc_axi_aw(ram_t *ram)
{
    if (ram->wr_active) {
        return;
    }
    if (itf_fifo_empty(ram->axi4_aw_slv)) {
        return;
    }

    axi4_aw_if_t aw;
    itf_read(ram->axi4_aw_slv, &aw);

    DBG_CHECK(aw.addr >= ram->base_addr);
    ram->wr_active = true;
    ram->wr_burst_addr = aw.addr - ram->base_addr;
    ram->wr_burst_id = aw.id;
    ram->wr_burst_len = aw.len;
    ram->wr_burst_size = aw.size;
    ram->wr_burst_type = aw.burst;
    ram->wr_burst_cnt = 0;
    ram->wr_b_pending = false;
}

static void ram_proc_axi_w(ram_t *ram)
{
    if (!ram->wr_active) {
        return;
    }
    if (itf_fifo_empty(ram->axi4_w_slv)) {
        return;
    }

    axi4_w_if_t w;
    itf_fifo_get_front(ram->axi4_w_slv, &w);

    if (w.last) {
        if (ram->wr_b_pending) {
            return;
        }
    }

    itf_fifo_pop_front(ram->axi4_w_slv);

    u32 addr = ram->wr_burst_addr;
    ram_write(ram, addr, w.data, w.strb);

    if (w.last) {
        ram->wr_active = false;
        ram->wr_b_pending = true;
    } else {
        ram->wr_burst_addr = axi_burst_next_addr(addr, ram->wr_burst_type,
                                                   ram->wr_burst_size);
        ram->wr_burst_cnt++;
    }
}

static void ram_proc_axi_b(ram_t *ram)
{
    if (!ram->wr_b_pending) {
        return;
    }
    if (itf_fifo_full(ram->axi4_b_mst)) {
        return;
    }

    axi4_b_if_t b = {
        .id = ram->wr_burst_id,
        .resp = AXI4_B_RESP_OKAY
    };
    itf_write(ram->axi4_b_mst, &b);
    ram->wr_b_pending = false;
}

static void ram_proc_axi(ram_t *ram)
{
    ram_proc_axi_b(ram);
    ram_proc_axi_r(ram);
    ram_proc_axi_w(ram);
    ram_proc_axi_aw(ram);
    ram_proc_axi_ar(ram);
}

static void ram_proc_port(ram_t *ram, u32 port_idx)
{
    DBG_CHECK(port_idx < ram->port_num);

    if (itf_fifo_empty(ram->bti_req_slvs[port_idx])) {
        return;
    }
    if (itf_fifo_full(ram->bti_rsp_msts[port_idx])) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(ram->bti_req_slvs[port_idx], &bti_req);
    DBG_CHECK(bti_req.addr >= ram->base_addr);

    bti_rsp_if_t bti_rsp;
    bti_rsp.trans_id = bti_req.trans_id;
    u32 addr = bti_req.addr - ram->base_addr;

    if (bti_req.cmd == BTI_REQ_CMD_READ) {
        bti_rsp.ok = ram_read(ram, addr, &bti_rsp.data);
    } else if (bti_req.cmd == BTI_REQ_CMD_WRITE) {
        bti_rsp.data = 0;
        bti_rsp.ok = ram_write(ram, addr, bti_req.data, bti_req.strobe);
    } else {
        bti_rsp.data = 0;
        bti_rsp.ok = false;
    }

    itf_write(ram->bti_rsp_msts[port_idx], &bti_rsp);
}

void ram_clock(ram_t *ram)
{
    if (ram->mode == RAM_MODE_AXI) {
        ram_proc_axi(ram);
    } else {
        for (u32 i = 0; i < ram->port_num; i++) {
            ram_proc_port(ram, i);
        }
    }
}
