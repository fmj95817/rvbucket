#include "l2.h"
#include <stdlib.h>
#include <string.h>
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

#define L2_WORD_SIZE 4u
#define L2_WORD_NUM (L2_LINE_SIZE / L2_WORD_SIZE)

static itf_t *l2_port_aw_slv(l2_t *l2, u32 port)
{
    return port == L2_I_PORT ? l2->i_axi4_aw_slv : l2->d_axi4_aw_slv;
}

static itf_t *l2_port_w_slv(l2_t *l2, u32 port)
{
    return port == L2_I_PORT ? l2->i_axi4_w_slv : l2->d_axi4_w_slv;
}

static itf_t *l2_port_b_mst(l2_t *l2, u32 port)
{
    return port == L2_I_PORT ? l2->i_axi4_b_mst : l2->d_axi4_b_mst;
}

static itf_t *l2_port_ar_slv(l2_t *l2, u32 port)
{
    return port == L2_I_PORT ? l2->i_axi4_ar_slv : l2->d_axi4_ar_slv;
}

static itf_t *l2_port_r_mst(l2_t *l2, u32 port)
{
    return port == L2_I_PORT ? l2->i_axi4_r_mst : l2->d_axi4_r_mst;
}

static u32 l2_line_idx(const l2_t *l2, u32 set, u32 way)
{
    return set * l2->conf.way_num + way;
}

static u32 l2_line_addr(const l2_t *l2, u32 tag, u32 set)
{
    return ((tag * l2->set_num) + set) * L2_LINE_SIZE;
}

static u32 l2_req_set(const l2_t *l2, u32 addr)
{
    return (addr / L2_LINE_SIZE) % l2->set_num;
}

static u32 l2_req_tag(const l2_t *l2, u32 addr)
{
    return (addr / L2_LINE_SIZE) / l2->set_num;
}

static bool l2_lookup(const l2_t *l2, u32 addr, u32 *line_idx)
{
    u32 set = l2_req_set(l2, addr);
    u32 tag = l2_req_tag(l2, addr);
    for (u32 way = 0; way < l2->conf.way_num; way++) {
        u32 idx = l2_line_idx(l2, set, way);
        if (l2->valids[idx] && l2->tags[idx] == tag) {
            *line_idx = idx;
            return true;
        }
    }
    return false;
}

static u32 l2_select_victim(l2_t *l2, u32 set)
{
    for (u32 way = 0; way < l2->conf.way_num; way++) {
        u32 idx = l2_line_idx(l2, set, way);
        if (!l2->valids[idx]) {
            return idx;
        }
    }

    u32 way = l2->replace_ways[set];
    l2->replace_ways[set] = (way + 1u) % l2->conf.way_num;
    return l2_line_idx(l2, set, way);
}

static u32 l2_next_addr(u32 addr, u32 size, u32 burst)
{
    DBG_CHECK(burst != AXI4_AR_BURST_WRAP);
    return burst == AXI4_AR_BURST_FIXED ? addr : addr + (1u << size);
}

static u32 l2_read_word(const l2_t *l2, u32 line_idx, u32 addr)
{
    u32 word = (addr & (L2_LINE_SIZE - 1u)) / L2_WORD_SIZE;
    return l2->data[line_idx * L2_WORD_NUM + word];
}

static void l2_write_word(l2_t *l2, u32 line_idx, u32 addr, u32 data, u8 strobe)
{
    u32 word = (addr & (L2_LINE_SIZE - 1u)) / L2_WORD_SIZE;
    u32 *dst = &l2->data[line_idx * L2_WORD_NUM + word];
    for (u32 byte = 0; byte < L2_WORD_SIZE; byte++) {
        if (strobe & (1u << byte)) {
            u32 shift = byte * 8u;
            *dst = (*dst & ~(0xffu << shift)) | (data & (0xffu << shift));
        }
    }
}

static void l2_port_next_read(l2_port_t *port)
{
    port->beat_idx++;
    port->addr = l2_next_addr(port->addr, port->ar.size, port->ar.burst);
}

static void l2_port_next_write(l2_port_t *port)
{
    port->beat_idx++;
    port->addr = l2_next_addr(port->addr, port->aw.size, port->aw.burst);
}

static bool l2_start_miss(l2_t *l2, u32 owner, l2_port_state_t return_state)
{
    if (l2->mem_state != L2_MEM_STATE_IDLE ||
        !ostq_empty(&l2->bypass_rd_ost) ||
        !ostq_empty(&l2->bypass_wr_ost)) {
        return false;
    }

    l2_port_t *port = &l2->ports[owner];
    l2->mem_owner = owner;
    l2->miss_line_addr = port->addr & ~(L2_LINE_SIZE - 1u);
    l2->miss_set = l2_req_set(l2, port->addr);
    l2->miss_tag = l2_req_tag(l2, port->addr);
    l2->miss_line_idx = l2_select_victim(l2, l2->miss_set);
    l2->wb_line_addr = l2_line_addr(l2, l2->tags[l2->miss_line_idx], l2->miss_set);
    l2->mem_beat_idx = 0;

    port->miss_return_state = return_state;
    port->state = L2_PORT_STATE_MISS_WAIT;
    (*l2->perf_miss)++;

    bool writeback = l2->valids[l2->miss_line_idx] && l2->dirtys[l2->miss_line_idx];
    l2->valids[l2->miss_line_idx] = false;
    if (writeback) {
        (*l2->perf_writeback)++;
        l2->mem_state = L2_MEM_STATE_WB_AW;
    } else {
        l2->mem_state = L2_MEM_STATE_REFILL_AR;
    }
    return true;
}

static void l2_proc_mem_wb_aw(l2_t *l2)
{
    if (l2->mem_state != L2_MEM_STATE_WB_AW || itf_fifo_full(l2->mem_axi4_aw_mst)) {
        return;
    }
    axi4_aw_if_t aw = {
        .id = 0,
        .addr = l2->wb_line_addr,
        .len = L2_WORD_NUM - 1u,
        .size = AXI4_AW_SIZE_B4,
        .burst = AXI4_AW_BURST_INCR,
        .cache = 0xf
    };
    itf_write(l2->mem_axi4_aw_mst, &aw);
    l2->mem_beat_idx = 0;
    l2->mem_state = L2_MEM_STATE_WB_W;
}

static void l2_proc_mem_wb_w(l2_t *l2)
{
    if (l2->mem_state != L2_MEM_STATE_WB_W || itf_fifo_full(l2->mem_axi4_w_mst)) {
        return;
    }
    axi4_w_if_t w = {
        .data = l2->data[l2->miss_line_idx * L2_WORD_NUM + l2->mem_beat_idx],
        .strb = 0xf,
        .last = l2->mem_beat_idx == L2_WORD_NUM - 1u
    };
    itf_write(l2->mem_axi4_w_mst, &w);
    if (w.last) {
        l2->mem_state = L2_MEM_STATE_WB_B;
    } else {
        l2->mem_beat_idx++;
    }
}

static void l2_proc_mem_wb_b(l2_t *l2)
{
    if (l2->mem_state != L2_MEM_STATE_WB_B || itf_fifo_empty(l2->mem_axi4_b_slv)) {
        return;
    }
    axi4_b_if_t b;
    itf_read(l2->mem_axi4_b_slv, &b);
    DBG_CHECK(b.resp == AXI4_B_RESP_OKAY);
    l2->dirtys[l2->miss_line_idx] = false;
    l2->mem_beat_idx = 0;
    l2->mem_state = L2_MEM_STATE_REFILL_AR;
}

static void l2_proc_mem_refill_ar(l2_t *l2)
{
    if (l2->mem_state != L2_MEM_STATE_REFILL_AR ||
        itf_fifo_full(l2->mem_axi4_ar_mst)) {
        return;
    }
    axi4_ar_if_t ar = {
        .id = 0,
        .addr = l2->miss_line_addr,
        .len = L2_WORD_NUM - 1u,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .cache = 0xf
    };
    itf_write(l2->mem_axi4_ar_mst, &ar);
    l2->mem_beat_idx = 0;
    l2->mem_state = L2_MEM_STATE_REFILL_R;
}

static void l2_proc_mem_refill_r(l2_t *l2)
{
    if (l2->mem_state != L2_MEM_STATE_REFILL_R ||
        itf_fifo_empty(l2->mem_axi4_r_slv)) {
        return;
    }
    axi4_r_if_t r;
    itf_read(l2->mem_axi4_r_slv, &r);
    DBG_CHECK(r.resp == AXI4_R_RESP_OKAY);
    l2->data[l2->miss_line_idx * L2_WORD_NUM + l2->mem_beat_idx] = r.data;
    l2->data_write_used = true;

    bool last = r.last || l2->mem_beat_idx == L2_WORD_NUM - 1u;
    if (last) {
        l2->tags[l2->miss_line_idx] = l2->miss_tag;
        l2->valids[l2->miss_line_idx] = true;
        l2->dirtys[l2->miss_line_idx] = false;
        l2_port_t *owner = &l2->ports[l2->mem_owner];
        owner->miss_resolved = true;
        owner->state = owner->miss_return_state;
        l2->mem_state = L2_MEM_STATE_IDLE;
    } else {
        l2->mem_beat_idx++;
    }
}

static void l2_proc_mem_bypass_r(l2_t *l2)
{
    if (itf_fifo_empty(l2->mem_axi4_r_slv)) {
        return;
    }

    l2_bypass_rd_ctx_t ctx;
    bool found = ostq_peek_head(&l2->bypass_rd_ost, &ctx, NULL);
    if (!found) {
        return;
    }

    itf_t *r_mst = l2_port_r_mst(l2, ctx.port);
    if (itf_fifo_full(r_mst)) {
        return;
    }

    axi4_r_if_t r;
    itf_read(l2->mem_axi4_r_slv, &r);
    itf_write(r_mst, &r);
    if (r.last) {
        ostq_free_head(&l2->bypass_rd_ost);
    }
}

static void l2_proc_mem_bypass_w(l2_t *l2)
{
    if (itf_fifo_full(l2->mem_axi4_w_mst)) {
        return;
    }

    l2_bypass_wr_ctx_t ctx;
    u32 slot;
    bool found = ostq_peek_head(&l2->bypass_wr_ost, &ctx, &slot);
    if (!found || ctx.w_done) {
        return;
    }

    fifo_t *w_fifo = &l2->w_fifos[ctx.port];
    if (fifo_empty(w_fifo)) {
        return;
    }

    axi4_w_if_t w;
    fifo_pop(w_fifo, &w);
    itf_write(l2->mem_axi4_w_mst, &w);
    ctx.beat_idx++;
    if (w.last || ctx.beat_idx >= ctx.beat_num) {
        ctx.w_done = true;
    }
    ostq_write_slot(&l2->bypass_wr_ost, slot, &ctx);
}

static void l2_proc_mem_bypass_b(l2_t *l2)
{
    if (itf_fifo_empty(l2->mem_axi4_b_slv)) {
        return;
    }

    l2_bypass_wr_ctx_t ctx;
    bool found = ostq_peek_head(&l2->bypass_wr_ost, &ctx, NULL);
    if (!found || !ctx.w_done) {
        return;
    }

    itf_t *b_mst = l2_port_b_mst(l2, ctx.port);
    if (itf_fifo_full(b_mst)) {
        return;
    }

    axi4_b_if_t b;
    itf_read(l2->mem_axi4_b_slv, &b);
    itf_write(b_mst, &b);
    ostq_free_head(&l2->bypass_wr_ost);
}

static void l2_proc_mem(l2_t *l2)
{
    l2_proc_mem_bypass_r(l2);
    l2_proc_mem_bypass_w(l2);
    l2_proc_mem_bypass_b(l2);
    l2_proc_mem_wb_b(l2);
    l2_proc_mem_refill_r(l2);
    l2_proc_mem_wb_w(l2);
    l2_proc_mem_wb_aw(l2);
    l2_proc_mem_refill_ar(l2);
}

static bool l2_start_bypass_read(l2_t *l2, u32 port_idx, const axi4_ar_if_t *ar)
{
    if (l2->mem_state != L2_MEM_STATE_IDLE ||
        ostq_full(&l2->bypass_rd_ost) ||
        itf_fifo_full(l2->mem_axi4_ar_mst)) {
        return false;
    }
    l2_bypass_rd_ctx_t ctx = { .port = port_idx };
    bool alloc_ok = ostq_alloc(&l2->bypass_rd_ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);
    itf_write(l2->mem_axi4_ar_mst, ar);
    (*l2->perf_bypass)++;
    return true;
}

static bool l2_start_bypass_write(l2_t *l2, u32 port_idx, const axi4_aw_if_t *aw)
{
    if (l2->mem_state != L2_MEM_STATE_IDLE ||
        ostq_full(&l2->bypass_wr_ost) ||
        itf_fifo_full(l2->mem_axi4_aw_mst)) {
        return false;
    }
    l2_bypass_wr_ctx_t ctx = {
        .port = port_idx,
        .beat_idx = 0,
        .beat_num = (u32)aw->len + 1u,
        .w_done = false
    };
    bool alloc_ok = ostq_alloc(&l2->bypass_wr_ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);
    itf_write(l2->mem_axi4_aw_mst, aw);
    (*l2->perf_bypass)++;
    return true;
}

static void l2_capture_port(l2_t *l2, u32 port_idx)
{
    itf_t *ar_slv = l2_port_ar_slv(l2, port_idx);
    if (fifo_full(&l2->ar_fifos[port_idx])) {
        if (!itf_fifo_empty(ar_slv)) {
            (*l2->perf_stg_ar_full[port_idx])++;
        }
    } else if (!itf_fifo_empty(ar_slv)) {
        axi4_ar_if_t ar;
        itf_read(ar_slv, &ar);
        fifo_push(&l2->ar_fifos[port_idx], &ar);
    }

    itf_t *aw_slv = l2_port_aw_slv(l2, port_idx);
    if (fifo_full(&l2->aw_fifos[port_idx])) {
        if (!itf_fifo_empty(aw_slv)) {
            (*l2->perf_stg_aw_full[port_idx])++;
        }
    } else if (!itf_fifo_empty(aw_slv)) {
        axi4_aw_if_t aw;
        itf_read(aw_slv, &aw);
        fifo_push(&l2->aw_fifos[port_idx], &aw);
    }

    itf_t *w_slv = l2_port_w_slv(l2, port_idx);
    if (fifo_full(&l2->w_fifos[port_idx])) {
        if (!itf_fifo_empty(w_slv)) {
            (*l2->perf_stg_w_full[port_idx])++;
        }
    } else if (!itf_fifo_empty(w_slv)) {
        axi4_w_if_t w;
        itf_read(w_slv, &w);
        fifo_push(&l2->w_fifos[port_idx], &w);
    }
}

static bool l2_port_has_bypass_pending(l2_t *l2, u32 port_idx)
{
    for (u32 i = 0; i < l2->bypass_rd_ost.depth; i++) {
        if (!ostq_slot_valid(&l2->bypass_rd_ost, i)) {
            continue;
        }
        l2_bypass_rd_ctx_t ctx;
        ostq_read_slot(&l2->bypass_rd_ost, i, &ctx);
        if (ctx.port == port_idx) {
            return true;
        }
    }

    for (u32 i = 0; i < l2->bypass_wr_ost.depth; i++) {
        if (!ostq_slot_valid(&l2->bypass_wr_ost, i)) {
            continue;
        }
        l2_bypass_wr_ctx_t ctx;
        ostq_read_slot(&l2->bypass_wr_ost, i, &ctx);
        if (ctx.port == port_idx) {
            return true;
        }
    }

    return false;
}

static void l2_proc_port_idle(l2_t *l2, u32 port_idx)
{
    l2_port_t *port = &l2->ports[port_idx];
    fifo_t *aw_fifo = &l2->aw_fifos[port_idx];
    if (!fifo_empty(aw_fifo)) {
        axi4_aw_if_t aw;
        fifo_get_front(aw_fifo, &aw);
        if (aw.cache == 0) {
            if (!l2_start_bypass_write(l2, port_idx, &aw)) {
                return;
            }
        } else {
            if (l2_port_has_bypass_pending(l2, port_idx)) {
                return;
            }
            DBG_CHECK(aw.size == AXI4_AW_SIZE_B4);
            DBG_CHECK(aw.burst == AXI4_AW_BURST_INCR);
            port->aw = aw;
            port->addr = aw.addr;
            port->beat_idx = 0;
            port->miss_resolved = false;
            port->state = L2_PORT_STATE_WRITE;
        }
        fifo_pop(aw_fifo, &aw);
        return;
    }

    fifo_t *ar_fifo = &l2->ar_fifos[port_idx];
    if (fifo_empty(ar_fifo)) {
        return;
    }
    axi4_ar_if_t ar;
    fifo_get_front(ar_fifo, &ar);
    if (ar.cache == 0) {
        if (!l2_start_bypass_read(l2, port_idx, &ar)) {
            return;
        }
    } else {
        if (l2_port_has_bypass_pending(l2, port_idx)) {
            return;
        }
        DBG_CHECK(ar.size == AXI4_AR_SIZE_B4);
        DBG_CHECK(ar.burst == AXI4_AR_BURST_INCR);
        port->ar = ar;
        port->addr = ar.addr;
        port->beat_idx = 0;
        port->miss_resolved = false;
        port->state = L2_PORT_STATE_READ;
    }
    fifo_pop(ar_fifo, &ar);
}

static void l2_proc_port_read(l2_t *l2, u32 port_idx)
{
    l2_port_t *port = &l2->ports[port_idx];
    itf_t *r_mst = l2_port_r_mst(l2, port_idx);
    if (itf_fifo_full(r_mst)) {
        return;
    }

    u32 line_idx;
    if (!l2_lookup(l2, port->addr, &line_idx)) {
        (void)l2_start_miss(l2, port_idx, L2_PORT_STATE_READ);
        return;
    }

    if (port->miss_resolved) {
        port->miss_resolved = false;
    } else {
        (*l2->perf_hit)++;
    }
    bool last = port->beat_idx == port->ar.len;
    axi4_r_if_t r = {
        .id = port->ar.id,
        .data = l2_read_word(l2, line_idx, port->addr),
        .resp = AXI4_R_RESP_OKAY,
        .last = last
    };
    itf_write(r_mst, &r);
    if (last) {
        port->state = L2_PORT_STATE_IDLE;
    } else {
        l2_port_next_read(port);
    }
}

static void l2_proc_port_write(l2_t *l2, u32 port_idx)
{
    l2_port_t *port = &l2->ports[port_idx];
    fifo_t *w_fifo = &l2->w_fifos[port_idx];
    if (fifo_empty(w_fifo)) {
        return;
    }

    u32 line_idx;
    if (!l2_lookup(l2, port->addr, &line_idx)) {
        (void)l2_start_miss(l2, port_idx, L2_PORT_STATE_WRITE);
        return;
    }
    if (l2->data_write_used) {
        return;
    }

    axi4_w_if_t w;
    fifo_get_front(w_fifo, &w);
    bool last = port->beat_idx == port->aw.len;
    DBG_CHECK(w.last == last);
    itf_t *b_mst = l2_port_b_mst(l2, port_idx);
    if (last && itf_fifo_full(b_mst)) {
        return;
    }
    fifo_pop(w_fifo, &w);
    l2_write_word(l2, line_idx, port->addr, w.data, w.strb);
    l2->dirtys[line_idx] = true;
    l2->data_write_used = true;
    if (port->miss_resolved) {
        port->miss_resolved = false;
    } else {
        (*l2->perf_hit)++;
    }

    if (last) {
        axi4_b_if_t b = { .id = port->aw.id, .resp = AXI4_B_RESP_OKAY };
        itf_write(b_mst, &b);
        port->state = L2_PORT_STATE_IDLE;
    } else {
        l2_port_next_write(port);
    }
}

static void l2_proc_port(l2_t *l2, u32 port_idx)
{
    l2_port_t *port = &l2->ports[port_idx];
    if (port->state == L2_PORT_STATE_IDLE) {
        l2_proc_port_idle(l2, port_idx);
    } else if (port->state == L2_PORT_STATE_READ) {
        l2_proc_port_read(l2, port_idx);
    } else if (port->state == L2_PORT_STATE_WRITE) {
        l2_proc_port_write(l2, port_idx);
    }
}

void l2_construct(l2_t *l2, const char *parent, const char *name,
    const l2_conf_t *conf)
{
    mod_construct(&l2->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf != NULL);
    DBG_CHECK(conf->way_num > 0);
    DBG_CHECK(conf->size >= L2_LINE_SIZE);
    DBG_CHECK((conf->size % (conf->way_num * L2_LINE_SIZE)) == 0);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->bypass_ost_depth > 0);
    DBG_CHECK(l2->i_axi4_aw_slv && l2->i_axi4_w_slv && l2->i_axi4_b_mst);
    DBG_CHECK(l2->i_axi4_ar_slv && l2->i_axi4_r_mst);
    DBG_CHECK(l2->d_axi4_aw_slv && l2->d_axi4_w_slv && l2->d_axi4_b_mst);
    DBG_CHECK(l2->d_axi4_ar_slv && l2->d_axi4_r_mst);
    DBG_CHECK(l2->mem_axi4_aw_mst && l2->mem_axi4_w_mst && l2->mem_axi4_b_slv);
    DBG_CHECK(l2->mem_axi4_ar_mst && l2->mem_axi4_r_slv);

    l2->conf = *conf;
    l2->set_num = conf->size / (conf->way_num * L2_LINE_SIZE);
    l2->line_num = l2->set_num * conf->way_num;
    l2->tags = malloc(sizeof(u32) * l2->line_num);
    l2->data = malloc(sizeof(u32) * l2->line_num * L2_WORD_NUM);
    l2->replace_ways = malloc(sizeof(u32) * l2->set_num);
    l2->valids = malloc(sizeof(bool) * l2->line_num);
    l2->dirtys = malloc(sizeof(bool) * l2->line_num);
    DBG_CHECK(l2->tags && l2->data && l2->replace_ways && l2->valids && l2->dirtys);
    for (u32 port = 0; port < L2_PORT_NUM; port++) {
        fifo_construct(&l2->ar_fifos[port], sizeof(axi4_ar_if_t),
            conf->stg_fifo_depth);
        fifo_construct(&l2->aw_fifos[port], sizeof(axi4_aw_if_t),
            conf->stg_fifo_depth);
        fifo_construct(&l2->w_fifos[port], sizeof(axi4_w_if_t),
            conf->stg_fifo_depth);
    }
    ostq_construct(&l2->bypass_rd_ost, sizeof(l2_bypass_rd_ctx_t),
        conf->bypass_ost_depth);
    ostq_construct(&l2->bypass_wr_ost, sizeof(l2_bypass_wr_ctx_t),
        conf->bypass_ost_depth);

    l2->perf_hit = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "hit");
    l2->perf_miss = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "miss");
    l2->perf_bypass = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "bypass");
    l2->perf_writeback = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "writeback");
    l2->perf_stg_ar_full[L2_I_PORT] = dbg_pcm_reg_perf_cnt(
        l2->mod.hier_name, "i_stg_ar_full");
    l2->perf_stg_aw_full[L2_I_PORT] = dbg_pcm_reg_perf_cnt(
        l2->mod.hier_name, "i_stg_aw_full");
    l2->perf_stg_w_full[L2_I_PORT] = dbg_pcm_reg_perf_cnt(
        l2->mod.hier_name, "i_stg_w_full");
    l2->perf_stg_ar_full[L2_D_PORT] = dbg_pcm_reg_perf_cnt(
        l2->mod.hier_name, "d_stg_ar_full");
    l2->perf_stg_aw_full[L2_D_PORT] = dbg_pcm_reg_perf_cnt(
        l2->mod.hier_name, "d_stg_aw_full");
    l2->perf_stg_w_full[L2_D_PORT] = dbg_pcm_reg_perf_cnt(
        l2->mod.hier_name, "d_stg_w_full");

    dbg_vcd_add_sig("i_state", DBG_SIG_TYPE_REG, 3, &l2->ports[L2_I_PORT].state);
    dbg_vcd_add_sig("d_state", DBG_SIG_TYPE_REG, 3, &l2->ports[L2_D_PORT].state);
    dbg_vcd_add_sig("mem_state", DBG_SIG_TYPE_REG, 4, &l2->mem_state);
    dbg_vcd_add_sig("i_addr", DBG_SIG_TYPE_REG, 32, &l2->ports[L2_I_PORT].addr);
    dbg_vcd_add_sig("d_addr", DBG_SIG_TYPE_REG, 32, &l2->ports[L2_D_PORT].addr);
    dbg_vcd_add_sig("bypass_rd_ost_count", DBG_SIG_TYPE_REG, 32,
        &l2->bypass_rd_ost.count);
    dbg_vcd_add_sig("bypass_wr_ost_count", DBG_SIG_TYPE_REG, 32,
        &l2->bypass_wr_ost.count);
}

void l2_reset(l2_t *l2)
{
    mod_reset(&l2->mod);
    for (u32 port = 0; port < L2_PORT_NUM; port++) {
        l2->ports[port].state = L2_PORT_STATE_IDLE;
        l2->ports[port].beat_idx = 0;
        l2->ports[port].miss_resolved = false;
        fifo_reset(&l2->ar_fifos[port]);
        fifo_reset(&l2->aw_fifos[port]);
        fifo_reset(&l2->w_fifos[port]);
    }
    l2->mem_state = L2_MEM_STATE_IDLE;
    l2->data_write_used = false;
    ostq_reset(&l2->bypass_rd_ost);
    ostq_reset(&l2->bypass_wr_ost);
    for (u32 i = 0; i < l2->line_num; i++) {
        l2->tags[i] = 0;
        l2->valids[i] = false;
        l2->dirtys[i] = false;
    }
    memset(l2->data, 0, sizeof(u32) * l2->line_num * L2_WORD_NUM);
    memset(l2->replace_ways, 0, sizeof(u32) * l2->set_num);
    *l2->perf_hit = 0;
    *l2->perf_miss = 0;
    *l2->perf_bypass = 0;
    *l2->perf_writeback = 0;
    for (u32 port = 0; port < L2_PORT_NUM; port++) {
        *l2->perf_stg_ar_full[port] = 0;
        *l2->perf_stg_aw_full[port] = 0;
        *l2->perf_stg_w_full[port] = 0;
    }
}

void l2_clock(l2_t *l2)
{
    mod_clock(&l2->mod);
    ostq_clock(&l2->bypass_rd_ost);
    ostq_clock(&l2->bypass_wr_ost);
    l2_capture_port(l2, L2_I_PORT);
    l2_capture_port(l2, L2_D_PORT);
    l2->data_write_used = false;
    l2_proc_mem(l2);
    l2_proc_port(l2, L2_I_PORT);
    l2_proc_port(l2, L2_D_PORT);
}

void l2_free(l2_t *l2)
{
    mod_free(&l2->mod);
    for (u32 port = 0; port < L2_PORT_NUM; port++) {
        fifo_free(&l2->ar_fifos[port]);
        fifo_free(&l2->aw_fifos[port]);
        fifo_free(&l2->w_fifos[port]);
    }
    ostq_free(&l2->bypass_rd_ost);
    ostq_free(&l2->bypass_wr_ost);
    free(l2->tags);
    free(l2->data);
    free(l2->replace_ways);
    free(l2->valids);
    free(l2->dirtys);
}
