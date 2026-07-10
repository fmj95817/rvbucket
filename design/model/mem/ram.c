#include "ram.h"
#include <stdlib.h>
#include <string.h>
#include "dbg/chk.h"
#include "dbg/vcd.h"

void ram_construct(ram_t *ram, const char *parent, const char *name, u32 port_num, ram_mode_t mode,
                   u32 size, u32 base_addr, u32 latency)
{
    mod_construct(&ram->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(port_num > 0);
    DBG_CHECK(port_num <= RAM_MAX_PORT_NUM);

    ram->port_num = port_num;
    ram->mode = mode;
    ram->size = size;
    ram->data = malloc(size + 3);
    ram->base_addr = base_addr;
    ram->latency = latency;
    DBG_CHECK(ram->data);
    for (u32 i = 0; i < RAM_MAX_PORT_NUM; i++) {
        ram->bti_entries[i] = NULL;
    }
    ram->axi_rd_entries = NULL;
    ram->axi_wr_entries = NULL;
    if (latency > 0) {
        if (mode == RAM_MODE_BTI) {
            for (u32 i = 0; i < port_num; i++) {
                ram->bti_entries[i] =
                    malloc(sizeof(*ram->bti_entries[i]) * RAM_PENDING_DEPTH);
                DBG_CHECK(ram->bti_entries[i]);
            }
        } else {
            ram->axi_rd_entries =
                malloc(sizeof(*ram->axi_rd_entries) * RAM_PENDING_DEPTH);
            ram->axi_wr_entries =
                malloc(sizeof(*ram->axi_wr_entries) * RAM_PENDING_DEPTH);
            DBG_CHECK(ram->axi_rd_entries);
            DBG_CHECK(ram->axi_wr_entries);
        }
    }

    dbg_vcd_add_sig("bti_count", DBG_SIG_TYPE_REG, 32, &ram->bti_count);
    dbg_vcd_add_sig("axi_rd_count", DBG_SIG_TYPE_REG, 32, &ram->axi_rd_count);
    dbg_vcd_add_sig("axi_wr_count", DBG_SIG_TYPE_REG, 32, &ram->axi_wr_count);
}

void ram_reset(ram_t *ram)
{
    mod_reset(&ram->mod);
    ram->rd_active = false;
    ram->wr_active = false;
    ram->wr_b_pending = false;
    for (u32 i = 0; i < RAM_MAX_PORT_NUM; i++) {
        ram->bti_rptrs[i] = 0;
        ram->bti_wptrs[i] = 0;
        ram->bti_counts[i] = 0;
    }
    ram->axi_rd_rptr = 0;
    ram->axi_rd_wptr = 0;
    ram->axi_wr_rptr = 0;
    ram->axi_wr_wptr = 0;
    ram->axi_wr_data_rptr = 0;
    ram->axi_wr_need_w_count = 0;
    ram->bti_count = 0;
    ram->axi_rd_count = 0;
    ram->axi_wr_count = 0;
}

void ram_free(ram_t *ram)
{
    mod_free(&ram->mod);
    free(ram->data);
    for (u32 i = 0; i < RAM_MAX_PORT_NUM; i++) {
        free(ram->bti_entries[i]);
    }
    free(ram->axi_rd_entries);
    free(ram->axi_wr_entries);
}

void ram_load(ram_t *ram, const void *data, u32 addr, u32 size)
{
    DBG_CHECK(addr < ram->size);
    DBG_CHECK(addr + size <= ram->size);
    memcpy(ram->data + addr, data, size);
}

static bool ram_read(ram_t *ram, u32 addr, u32 byte_num, u32 *data)
{
    if (byte_num > sizeof(*data) || addr >= ram->size || byte_num > ram->size - addr) {
        return false;
    }
    *data = 0;
    memcpy(data, ram->data + addr, byte_num);
    return true;
}

static bool ram_write(ram_t *ram, u32 addr, u32 byte_num, u32 data, u8 strobe)
{
    if (byte_num > sizeof(data) || addr >= ram->size || byte_num > ram->size - addr) {
        return false;
    }

    u8 *dst = (ram->data + addr);

    for (u32 i = 0; i < byte_num; i++) {
        if (strobe & (1u << i)) {
            dst[i] = (u8)(data >> (i * 8u));
        }
    }

    return true;
}

static u32 axi_burst_next_addr(u32 addr, u32 burst_type, u32 burst_size)
{
    u32 byte_step = (u32)1 << burst_size;
    if (burst_type == AXI4_AR_BURST_FIXED) {
        return addr;
    } else {
        return addr + byte_step;
    }
}

static u32 ram_fifo_next(u32 ptr)
{
    return (ptr + 1u) % RAM_PENDING_DEPTH;
}

static u64 ram_cycle(const ram_t *ram)
{
    return ram->mod.cycle == NULL ? 0 : *ram->mod.cycle;
}

static u64 ram_ready_cycle(const ram_t *ram)
{
    return ram_cycle(ram) + ram->latency;
}

static bool ram_entry_ready(const ram_t *ram, u64 ready_cycle)
{
    return ram_cycle(ram) >= ready_cycle;
}

static bti_rsp_if_t ram_build_bti_rsp(ram_t *ram, const bti_req_if_t *req)
{
    bti_rsp_if_t rsp = {
        .trans_id = req->trans_id,
        .data = 0,
        .ok = false
    };
    u32 addr = req->addr - ram->base_addr;
    u32 byte_num = 1u << req->size;

    if (req->size > BTI_REQ_SIZE_B4) {
        rsp.ok = false;
    } else if (req->cmd == BTI_REQ_CMD_READ) {
        rsp.ok = ram_read(ram, addr, byte_num, &rsp.data);
    } else if (req->cmd == BTI_REQ_CMD_WRITE) {
        rsp.ok = ram_write(ram, addr, byte_num, req->data, req->strobe);
    }
    return rsp;
}

static void ram_fast_proc_axi_ar(ram_t *ram)
{
    if (ram->rd_active || itf_fifo_empty(ram->axi4_ar_slv)) {
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

static void ram_fast_proc_axi_r(ram_t *ram)
{
    if (!ram->rd_active || itf_fifo_full(ram->axi4_r_mst)) {
        return;
    }

    u32 data = 0;
    u32 byte_num = 1u << ram->rd_burst_size;
    bool ok = ram_read(ram, ram->rd_burst_addr, byte_num, &data);
    bool last = ram->rd_burst_cnt == ram->rd_burst_len;

    axi4_r_if_t r = {
        .id = ram->rd_burst_id,
        .data = data,
        .resp = ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR,
        .last = last
    };
    itf_write(ram->axi4_r_mst, &r);

    if (last) {
        ram->rd_active = false;
    } else {
        ram->rd_burst_addr = axi_burst_next_addr(ram->rd_burst_addr,
            ram->rd_burst_type, ram->rd_burst_size);
        ram->rd_burst_cnt++;
    }
}

static void ram_fast_proc_axi_aw(ram_t *ram)
{
    if (ram->wr_active || itf_fifo_empty(ram->axi4_aw_slv)) {
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

static void ram_fast_proc_axi_w(ram_t *ram)
{
    if (!ram->wr_active || itf_fifo_empty(ram->axi4_w_slv)) {
        return;
    }

    axi4_w_if_t w;
    itf_fifo_get_front(ram->axi4_w_slv, &w);

    bool last = w.last || ram->wr_burst_cnt == ram->wr_burst_len;
    if (last && ram->wr_b_pending) {
        return;
    }

    itf_fifo_pop_front(ram->axi4_w_slv);
    u32 byte_num = 1u << ram->wr_burst_size;
    ram_write(ram, ram->wr_burst_addr, byte_num, w.data, w.strb);

    if (last) {
        ram->wr_active = false;
        ram->wr_b_pending = true;
    } else {
        ram->wr_burst_addr = axi_burst_next_addr(ram->wr_burst_addr,
            ram->wr_burst_type, ram->wr_burst_size);
        ram->wr_burst_cnt++;
    }
}

static void ram_fast_proc_axi_b(ram_t *ram)
{
    if (!ram->wr_b_pending || itf_fifo_full(ram->axi4_b_mst)) {
        return;
    }

    axi4_b_if_t b = {
        .id = ram->wr_burst_id,
        .resp = AXI4_B_RESP_OKAY
    };
    itf_write(ram->axi4_b_mst, &b);
    ram->wr_b_pending = false;
}

static void ram_fast_proc_axi(ram_t *ram)
{
    ram_fast_proc_axi_b(ram);
    ram_fast_proc_axi_r(ram);
    ram_fast_proc_axi_w(ram);
    ram_fast_proc_axi_aw(ram);
    ram_fast_proc_axi_ar(ram);
}

static void ram_fast_proc_bti_port(ram_t *ram, u32 port_idx)
{
    DBG_CHECK(port_idx < ram->port_num);
    if (itf_fifo_empty(ram->bti_req_slvs[port_idx]) ||
        itf_fifo_full(ram->bti_rsp_msts[port_idx])) {
        return;
    }

    bti_req_if_t req;
    itf_read(ram->bti_req_slvs[port_idx], &req);
    DBG_CHECK(req.addr >= ram->base_addr);

    bti_rsp_if_t rsp = ram_build_bti_rsp(ram, &req);
    itf_write(ram->bti_rsp_msts[port_idx], &rsp);
}

static void ram_fast_proc_bti(ram_t *ram)
{
    for (u32 i = 0; i < ram->port_num; i++) {
        ram_fast_proc_bti_port(ram, i);
    }
}

static void ram_send_bti_rsp(ram_t *ram, u32 port_idx)
{
    if (ram->bti_counts[port_idx] == 0 ||
        itf_fifo_full(ram->bti_rsp_msts[port_idx])) {
        return;
    }

    ram_bti_entry_t *entry =
        &ram->bti_entries[port_idx][ram->bti_rptrs[port_idx]];
    if (!ram_entry_ready(ram, entry->ready_cycle)) {
        return;
    }

    itf_write(ram->bti_rsp_msts[port_idx], &entry->rsp);
    ram->bti_rptrs[port_idx] = ram_fifo_next(ram->bti_rptrs[port_idx]);
    ram->bti_counts[port_idx]--;
    ram->bti_count--;
}

static void ram_accept_bti_req(ram_t *ram, u32 port_idx)
{
    if (itf_fifo_empty(ram->bti_req_slvs[port_idx])) {
        return;
    }

    if (ram->bti_counts[port_idx] == RAM_PENDING_DEPTH) {
        return;
    }

    bti_req_if_t req;
    itf_read(ram->bti_req_slvs[port_idx], &req);
    DBG_CHECK(req.addr >= ram->base_addr);

    ram_bti_entry_t *entry =
        &ram->bti_entries[port_idx][ram->bti_wptrs[port_idx]];
    entry->ready_cycle = ram_ready_cycle(ram);
    entry->rsp = ram_build_bti_rsp(ram, &req);
    ram->bti_wptrs[port_idx] = ram_fifo_next(ram->bti_wptrs[port_idx]);
    ram->bti_counts[port_idx]++;
    ram->bti_count++;
}

static void ram_proc_bti(ram_t *ram)
{
    for (u32 i = 0; i < ram->port_num; i++) {
        ram_send_bti_rsp(ram, i);
    }
    for (u32 i = 0; i < ram->port_num; i++) {
        ram_accept_bti_req(ram, i);
    }
}

static void ram_accept_axi_ar(ram_t *ram)
{
    if (itf_fifo_empty(ram->axi4_ar_slv)) {
        return;
    }

    if (ram->axi_rd_count == RAM_PENDING_DEPTH) {
        return;
    }

    axi4_ar_if_t ar;
    itf_read(ram->axi4_ar_slv, &ar);
    DBG_CHECK(ar.addr >= ram->base_addr);

    ram_axi_rd_entry_t *entry = &ram->axi_rd_entries[ram->axi_rd_wptr];
    entry->ready_cycle = ram_ready_cycle(ram);
    entry->addr = ar.addr - ram->base_addr;
    entry->id = ar.id;
    entry->len = ar.len;
    entry->size = ar.size;
    entry->burst = ar.burst;
    entry->beat_cnt = 0;
    ram->axi_rd_wptr = ram_fifo_next(ram->axi_rd_wptr);
    ram->axi_rd_count++;
}

static void ram_send_axi_r(ram_t *ram)
{
    if (ram->axi_rd_count == 0 || itf_fifo_full(ram->axi4_r_mst)) {
        return;
    }

    ram_axi_rd_entry_t *entry = &ram->axi_rd_entries[ram->axi_rd_rptr];
    if (!ram_entry_ready(ram, entry->ready_cycle)) {
        return;
    }

    u32 data = 0;
    u32 byte_num = 1u << entry->size;
    bool ok = ram_read(ram, entry->addr, byte_num, &data);
    bool last = entry->beat_cnt == entry->len;

    axi4_r_if_t r = {
        .id = entry->id,
        .data = data,
        .resp = ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR,
        .last = last
    };
    itf_write(ram->axi4_r_mst, &r);

    if (last) {
        ram->axi_rd_rptr = ram_fifo_next(ram->axi_rd_rptr);
        ram->axi_rd_count--;
    } else {
        entry->addr = axi_burst_next_addr(entry->addr, entry->burst,
            entry->size);
        entry->beat_cnt++;
    }
}

static void ram_accept_axi_aw(ram_t *ram)
{
    if (itf_fifo_empty(ram->axi4_aw_slv)) {
        return;
    }

    if (ram->axi_wr_count == RAM_PENDING_DEPTH) {
        return;
    }

    axi4_aw_if_t aw;
    itf_read(ram->axi4_aw_slv, &aw);
    DBG_CHECK(aw.addr >= ram->base_addr);

    u32 slot = ram->axi_wr_wptr;
    ram_axi_wr_entry_t *entry = &ram->axi_wr_entries[slot];
    entry->ready_cycle = 0;
    entry->addr = aw.addr - ram->base_addr;
    entry->id = aw.id;
    entry->len = aw.len;
    entry->size = aw.size;
    entry->burst = aw.burst;
    entry->beat_cnt = 0;
    entry->w_done = false;
    if (ram->axi_wr_need_w_count == 0) {
        ram->axi_wr_data_rptr = slot;
    }
    ram->axi_wr_wptr = ram_fifo_next(ram->axi_wr_wptr);
    ram->axi_wr_count++;
    ram->axi_wr_need_w_count++;
}

static void ram_accept_axi_w(ram_t *ram)
{
    if (itf_fifo_empty(ram->axi4_w_slv)) {
        return;
    }

    if (ram->axi_wr_need_w_count == 0) {
        return;
    }

    ram_axi_wr_entry_t *entry =
        &ram->axi_wr_entries[ram->axi_wr_data_rptr];
    DBG_CHECK(!entry->w_done);
    axi4_w_if_t w;
    itf_fifo_get_front(ram->axi4_w_slv, &w);

    bool last = w.last || entry->beat_cnt == entry->len;
    itf_fifo_pop_front(ram->axi4_w_slv);

    u32 byte_num = 1u << entry->size;
    ram_write(ram, entry->addr, byte_num, w.data, w.strb);

    if (last) {
        entry->w_done = true;
        entry->ready_cycle = ram_ready_cycle(ram);
        ram->axi_wr_need_w_count--;
        if (ram->axi_wr_need_w_count > 0) {
            ram->axi_wr_data_rptr = ram_fifo_next(ram->axi_wr_data_rptr);
        }
    } else {
        entry->addr = axi_burst_next_addr(entry->addr, entry->burst,
            entry->size);
        entry->beat_cnt++;
    }
}

static void ram_send_axi_b(ram_t *ram)
{
    if (ram->axi_wr_count == 0 || itf_fifo_full(ram->axi4_b_mst)) {
        return;
    }

    ram_axi_wr_entry_t *entry = &ram->axi_wr_entries[ram->axi_wr_rptr];
    if (!entry->w_done || !ram_entry_ready(ram, entry->ready_cycle)) {
        return;
    }

    axi4_b_if_t b = {
        .id = entry->id,
        .resp = AXI4_B_RESP_OKAY
    };
    itf_write(ram->axi4_b_mst, &b);
    ram->axi_wr_rptr = ram_fifo_next(ram->axi_wr_rptr);
    ram->axi_wr_count--;
}

static void ram_proc_axi(ram_t *ram)
{
    ram_send_axi_b(ram);
    ram_send_axi_r(ram);
    ram_accept_axi_w(ram);
    ram_accept_axi_aw(ram);
    ram_accept_axi_ar(ram);
}

void ram_clock(ram_t *ram)
{
    mod_clock(&ram->mod);
    if (ram->latency == 0) {
        if (ram->mode == RAM_MODE_AXI) {
            ram_fast_proc_axi(ram);
        } else {
            ram_fast_proc_bti(ram);
        }
        return;
    }

    if (ram->mode == RAM_MODE_AXI) {
        ram_proc_axi(ram);
    } else {
        ram_proc_bti(ram);
    }
}
