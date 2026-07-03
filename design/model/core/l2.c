#include "l2.h"
#include <stdlib.h>
#include <string.h>
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

#define L2_WORD_SIZE 4u
#define L2_WORD_NUM (L2_LINE_SIZE / L2_WORD_SIZE)
#define L2_AXI_FIFO_DEPTH 2u

static u32 l2_line_idx(const l2_t *l2, u32 set, u32 way)
{
    return set * l2->conf.way_num + way;
}

static u32 l2_line_addr(const l2_t *l2, u32 tag, u32 set)
{
    return ((tag * l2->set_num) + set) * L2_LINE_SIZE;
}

static u32 l2_req_line_addr(u32 addr)
{
    return addr & ~(L2_LINE_SIZE - 1u);
}

static u32 l2_req_set(const l2_t *l2, u32 addr)
{
    return (l2_req_line_addr(addr) / L2_LINE_SIZE) % l2->set_num;
}

static u32 l2_req_tag(const l2_t *l2, u32 addr)
{
    return (l2_req_line_addr(addr) / L2_LINE_SIZE) / l2->set_num;
}

static u32 l2_beat_size(const l2_t *l2)
{
    u32 size = l2->is_write ? l2->aw.size : l2->ar.size;
    DBG_CHECK(size <= AXI4_AR_SIZE_B4);
    return 1u << size;
}

static u32 l2_next_addr(u32 addr, u32 size, u32 burst)
{
    DBG_CHECK(burst != AXI4_AR_BURST_WRAP);
    return burst == AXI4_AR_BURST_FIXED ? addr : addr + (1u << size);
}

static u8 l2_read_byte(const l2_t *l2, u32 line_idx, u32 line_offset)
{
    u32 word = l2->data[line_idx * L2_WORD_NUM + line_offset / L2_WORD_SIZE];
    return (u8)(word >> ((line_offset % L2_WORD_SIZE) * 8u));
}

static void l2_write_byte(l2_t *l2, u32 line_idx, u32 line_offset, u8 data)
{
    u32 *word = &l2->data[line_idx * L2_WORD_NUM + line_offset / L2_WORD_SIZE];
    u32 shift = (line_offset % L2_WORD_SIZE) * 8u;
    *word = (*word & ~(0xffu << shift)) | ((u32)data << shift);
}

static bool l2_lookup(l2_t *l2, u32 *way, u32 *line_idx)
{
    for (u32 i = 0; i < l2->conf.way_num; i++) {
        u32 idx = l2_line_idx(l2, l2->req_set, i);
        if (l2->valids[idx] && l2->tags[idx] == l2->req_tag) {
            *way = i;
            *line_idx = idx;
            return true;
        }
    }
    return false;
}

static void l2_select_victim(l2_t *l2)
{
    for (u32 i = 0; i < l2->conf.way_num; i++) {
        u32 idx = l2_line_idx(l2, l2->req_set, i);
        if (!l2->valids[idx]) {
            l2->req_way = i;
            l2->req_line_idx = idx;
            return;
        }
    }

    l2->req_way = l2->replace_ways[l2->req_set];
    l2->req_line_idx = l2_line_idx(l2, l2->req_set, l2->req_way);
    l2->replace_ways[l2->req_set] = (l2->req_way + 1u) % l2->conf.way_num;
}

static void l2_start_fragment(l2_t *l2)
{
    u32 addr = l2->burst_addr + l2->byte_idx;
    l2->req_line_addr = l2_req_line_addr(addr);
    l2->req_set = l2_req_set(l2, addr);
    l2->req_tag = l2_req_tag(l2, addr);

    u32 way;
    u32 line_idx;
    if (l2_lookup(l2, &way, &line_idx)) {
        (*l2->perf_hit)++;
        l2->req_way = way;
        l2->req_line_idx = line_idx;
        l2->state = l2->is_write ? L2_STATE_WRITE_SERVE : L2_STATE_READ_SERVE;
        return;
    }

    (*l2->perf_miss)++;
    l2_select_victim(l2);
    l2->line_beat_idx = 0;
    if (l2->valids[l2->req_line_idx] && l2->dirtys[l2->req_line_idx]) {
        (*l2->perf_writeback)++;
        l2->wb_line_addr = l2_line_addr(l2, l2->tags[l2->req_line_idx], l2->req_set);
        l2->state = L2_STATE_WB_AW;
    } else {
        l2->state = L2_STATE_REFILL_AR;
    }
}

static void l2_start_read_beat(l2_t *l2)
{
    l2->byte_idx = 0;
    l2->beat_data = 0;
    l2->beat_resp = AXI4_R_RESP_OKAY;
    l2_start_fragment(l2);
}

static void l2_finish_read_beat(l2_t *l2)
{
    if (itf_fifo_full(&l2->merged_axi4_r_itf)) {
        return;
    }

    bool last = l2->beat_idx == l2->ar.len;
    axi4_r_if_t r = {
        .id = l2->ar.id,
        .data = l2->beat_data,
        .resp = (axi4_r_resp_t)l2->beat_resp,
        .last = last
    };
    itf_write(&l2->merged_axi4_r_itf, &r);
    if (last) {
        l2->state = L2_STATE_IDLE;
        return;
    }

    l2->beat_idx++;
    l2->burst_addr = l2_next_addr(l2->burst_addr, l2->ar.size, l2->ar.burst);
    l2_start_read_beat(l2);
}

static void l2_proc_read_serve(l2_t *l2)
{
    if (l2->state != L2_STATE_READ_SERVE) {
        return;
    }

    u32 size = l2_beat_size(l2);
    if (l2->byte_idx == size) {
        l2_finish_read_beat(l2);
        return;
    }

    u32 addr = l2->burst_addr + l2->byte_idx;
    u32 line_offset = addr & (L2_LINE_SIZE - 1u);
    u32 byte_num = MIN(size - l2->byte_idx, L2_LINE_SIZE - line_offset);
    for (u32 i = 0; i < byte_num; i++) {
        l2->beat_data |= (u32)l2_read_byte(l2, l2->req_line_idx, line_offset + i)
            << ((l2->byte_idx + i) * 8u);
    }
    l2->byte_idx += byte_num;
    if (l2->byte_idx < size) {
        l2_start_fragment(l2);
    }
}

static void l2_finish_write_beat(l2_t *l2)
{
    bool last = l2->beat_idx == l2->aw.len;
    DBG_CHECK(l2->w.last == last);
    if (last) {
        if (itf_fifo_full(&l2->merged_axi4_b_itf)) {
            return;
        }
        axi4_b_if_t b = {
            .id = l2->aw.id,
            .resp = (axi4_b_resp_t)l2->write_resp
        };
        itf_write(&l2->merged_axi4_b_itf, &b);
        l2->state = L2_STATE_IDLE;
        return;
    }

    l2->beat_idx++;
    l2->burst_addr = l2_next_addr(l2->burst_addr, l2->aw.size, l2->aw.burst);
    l2->state = L2_STATE_WRITE_WAIT_W;
}

static void l2_proc_write_wait_w(l2_t *l2)
{
    if (l2->state != L2_STATE_WRITE_WAIT_W ||
        itf_fifo_empty(&l2->merged_axi4_w_itf)) {
        return;
    }
    itf_read(&l2->merged_axi4_w_itf, &l2->w);
    l2->byte_idx = 0;
    l2_start_fragment(l2);
}

static void l2_proc_write_serve(l2_t *l2)
{
    if (l2->state != L2_STATE_WRITE_SERVE) {
        return;
    }

    u32 size = l2_beat_size(l2);
    if (l2->byte_idx == size) {
        l2_finish_write_beat(l2);
        return;
    }

    u32 addr = l2->burst_addr + l2->byte_idx;
    u32 line_offset = addr & (L2_LINE_SIZE - 1u);
    u32 byte_num = MIN(size - l2->byte_idx, L2_LINE_SIZE - line_offset);
    for (u32 i = 0; i < byte_num; i++) {
        u32 byte_idx = l2->byte_idx + i;
        if (l2->w.strb & (1u << byte_idx)) {
            l2_write_byte(l2, l2->req_line_idx, line_offset + i,
                (u8)(l2->w.data >> (byte_idx * 8u)));
            l2->dirtys[l2->req_line_idx] = true;
        }
    }
    l2->byte_idx += byte_num;
    if (l2->byte_idx < size) {
        l2_start_fragment(l2);
    }
}

static void l2_proc_wb_aw(l2_t *l2)
{
    if (l2->state != L2_STATE_WB_AW || itf_fifo_full(l2->gst_axi4_aw_mst)) {
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
    itf_write(l2->gst_axi4_aw_mst, &aw);
    l2->line_beat_idx = 0;
    l2->state = L2_STATE_WB_W;
}

static void l2_proc_wb_w(l2_t *l2)
{
    if (l2->state != L2_STATE_WB_W || itf_fifo_full(l2->gst_axi4_w_mst)) {
        return;
    }
    axi4_w_if_t w = {
        .data = l2->data[l2->req_line_idx * L2_WORD_NUM + l2->line_beat_idx],
        .strb = 0xf,
        .last = l2->line_beat_idx == L2_WORD_NUM - 1u
    };
    itf_write(l2->gst_axi4_w_mst, &w);
    if (w.last) {
        l2->state = L2_STATE_WB_B;
    } else {
        l2->line_beat_idx++;
    }
}

static void l2_proc_wb_b(l2_t *l2)
{
    if (l2->state != L2_STATE_WB_B || itf_fifo_empty(l2->gst_axi4_b_slv)) {
        return;
    }
    axi4_b_if_t b;
    itf_read(l2->gst_axi4_b_slv, &b);
    if (b.resp != AXI4_B_RESP_OKAY) {
        l2->write_resp = b.resp;
        l2->beat_resp = AXI4_R_RESP_SLVERR;
    }
    l2->dirtys[l2->req_line_idx] = false;
    l2->line_beat_idx = 0;
    l2->state = L2_STATE_REFILL_AR;
}

static void l2_proc_refill_ar(l2_t *l2)
{
    if (l2->state != L2_STATE_REFILL_AR || itf_fifo_full(l2->gst_axi4_ar_mst)) {
        return;
    }
    axi4_ar_if_t ar = {
        .id = 0,
        .addr = l2->req_line_addr,
        .len = L2_WORD_NUM - 1u,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .cache = 0xf
    };
    itf_write(l2->gst_axi4_ar_mst, &ar);
    l2->line_beat_idx = 0;
    l2->state = L2_STATE_REFILL_R;
}

static void l2_proc_refill_r(l2_t *l2)
{
    if (l2->state != L2_STATE_REFILL_R || itf_fifo_empty(l2->gst_axi4_r_slv)) {
        return;
    }
    axi4_r_if_t r;
    itf_read(l2->gst_axi4_r_slv, &r);
    if (r.resp != AXI4_R_RESP_OKAY) {
        l2->beat_resp = r.resp;
        l2->write_resp = AXI4_B_RESP_SLVERR;
    }
    l2->data[l2->req_line_idx * L2_WORD_NUM + l2->line_beat_idx] = r.data;

    bool last = r.last || l2->line_beat_idx == L2_WORD_NUM - 1u;
    if (last) {
        l2->valids[l2->req_line_idx] = l2->beat_resp == AXI4_R_RESP_OKAY;
        l2->dirtys[l2->req_line_idx] = false;
        l2->tags[l2->req_line_idx] = l2->req_tag;
        l2->state = l2->is_write ? L2_STATE_WRITE_SERVE : L2_STATE_READ_SERVE;
    } else {
        l2->line_beat_idx++;
    }
}

static void l2_proc_bypass_r(l2_t *l2)
{
    if (l2->state != L2_STATE_BYPASS_R || itf_fifo_empty(l2->gst_axi4_r_slv) ||
        itf_fifo_full(&l2->merged_axi4_r_itf)) {
        return;
    }
    axi4_r_if_t r;
    itf_read(l2->gst_axi4_r_slv, &r);
    itf_write(&l2->merged_axi4_r_itf, &r);
    if (r.last) {
        l2->state = L2_STATE_IDLE;
    }
}

static void l2_proc_bypass_w(l2_t *l2)
{
    if (l2->state != L2_STATE_BYPASS_W ||
        itf_fifo_empty(&l2->merged_axi4_w_itf) || itf_fifo_full(l2->gst_axi4_w_mst)) {
        return;
    }
    axi4_w_if_t w;
    itf_read(&l2->merged_axi4_w_itf, &w);
    itf_write(l2->gst_axi4_w_mst, &w);
    if (w.last) {
        l2->state = L2_STATE_BYPASS_B;
    }
}

static void l2_proc_bypass_b(l2_t *l2)
{
    if (l2->state != L2_STATE_BYPASS_B || itf_fifo_empty(l2->gst_axi4_b_slv) ||
        itf_fifo_full(&l2->merged_axi4_b_itf)) {
        return;
    }
    axi4_b_if_t b;
    itf_read(l2->gst_axi4_b_slv, &b);
    itf_write(&l2->merged_axi4_b_itf, &b);
    l2->state = L2_STATE_IDLE;
}

static void l2_proc_idle(l2_t *l2)
{
    if (l2->state != L2_STATE_IDLE) {
        return;
    }

    if (!itf_fifo_empty(&l2->merged_axi4_aw_itf)) {
        itf_fifo_get_front(&l2->merged_axi4_aw_itf, &l2->aw);
        if (l2->aw.cache == 0) {
            if (itf_fifo_full(l2->gst_axi4_aw_mst)) {
                return;
            }
            itf_fifo_pop_front(&l2->merged_axi4_aw_itf);
            itf_write(l2->gst_axi4_aw_mst, &l2->aw);
            (*l2->perf_bypass)++;
            l2->state = L2_STATE_BYPASS_W;
            return;
        }

        DBG_CHECK(l2->aw.size <= AXI4_AW_SIZE_B4);
        DBG_CHECK(l2->aw.burst != AXI4_AW_BURST_WRAP);
        itf_fifo_pop_front(&l2->merged_axi4_aw_itf);
        l2->is_write = true;
        l2->burst_addr = l2->aw.addr;
        l2->beat_idx = 0;
        l2->write_resp = AXI4_B_RESP_OKAY;
        l2->state = L2_STATE_WRITE_WAIT_W;
        return;
    }

    if (!itf_fifo_empty(&l2->merged_axi4_ar_itf)) {
        itf_fifo_get_front(&l2->merged_axi4_ar_itf, &l2->ar);
        if (l2->ar.cache == 0) {
            if (itf_fifo_full(l2->gst_axi4_ar_mst)) {
                return;
            }
            itf_fifo_pop_front(&l2->merged_axi4_ar_itf);
            itf_write(l2->gst_axi4_ar_mst, &l2->ar);
            (*l2->perf_bypass)++;
            l2->state = L2_STATE_BYPASS_R;
            return;
        }

        DBG_CHECK(l2->ar.size <= AXI4_AR_SIZE_B4);
        DBG_CHECK(l2->ar.burst != AXI4_AR_BURST_WRAP);
        itf_fifo_pop_front(&l2->merged_axi4_ar_itf);
        l2->is_write = false;
        l2->burst_addr = l2->ar.addr;
        l2->beat_idx = 0;
        l2_start_read_beat(l2);
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

    AXI4_IF_CONSTRUCT(l2, merged_, L2_AXI_FIFO_DEPTH);
    for (u32 i = 0; i < L2_HOST_NUM; i++) {
        l2->axi_mux.host_axi4_aw_slvs[i] = l2->host_axi4_aw_slvs[i];
        l2->axi_mux.host_axi4_w_slvs[i] = l2->host_axi4_w_slvs[i];
        l2->axi_mux.host_axi4_b_msts[i] = l2->host_axi4_b_msts[i];
        l2->axi_mux.host_axi4_ar_slvs[i] = l2->host_axi4_ar_slvs[i];
        l2->axi_mux.host_axi4_r_msts[i] = l2->host_axi4_r_msts[i];
    }
    AXI4_MST_CONNECT(&l2->axi_mux, gst_, l2, merged_);
    l2->axi_mux.mod.cycle = l2->mod.cycle;
    axi_mux_construct(&l2->axi_mux, l2->mod.hier_name, "u_axi_mux", L2_HOST_NUM);

    l2->conf = *conf;
    l2->set_num = conf->size / (conf->way_num * L2_LINE_SIZE);
    l2->line_num = l2->set_num * conf->way_num;
    l2->tags = malloc(sizeof(u32) * l2->line_num);
    l2->data = malloc(sizeof(u32) * l2->line_num * L2_WORD_NUM);
    l2->replace_ways = malloc(sizeof(u32) * l2->set_num);
    l2->valids = malloc(sizeof(bool) * l2->line_num);
    l2->dirtys = malloc(sizeof(bool) * l2->line_num);
    DBG_CHECK(l2->tags && l2->data && l2->replace_ways && l2->valids && l2->dirtys);

    l2->perf_hit = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "hit");
    l2->perf_miss = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "miss");
    l2->perf_bypass = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "bypass");
    l2->perf_writeback = dbg_pcm_reg_perf_cnt(l2->mod.hier_name, "writeback");

    dbg_vcd_add_sig("state", DBG_SIG_TYPE_REG, 4, &l2->state);
    dbg_vcd_add_sig("burst_addr", DBG_SIG_TYPE_REG, 32, &l2->burst_addr);
    dbg_vcd_add_sig("req_set", DBG_SIG_TYPE_REG, 32, &l2->req_set);
    dbg_vcd_add_sig("req_way", DBG_SIG_TYPE_REG, 32, &l2->req_way);
}

void l2_reset(l2_t *l2)
{
    mod_reset(&l2->mod);
    axi_mux_reset(&l2->axi_mux);
    AXI4_IF_RESET(l2, merged_);
    l2->state = L2_STATE_IDLE;
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
}

void l2_clock(l2_t *l2)
{
    mod_clock(&l2->mod);
    axi_mux_clock(&l2->axi_mux);

    l2_proc_bypass_r(l2);
    l2_proc_bypass_w(l2);
    l2_proc_bypass_b(l2);
    l2_proc_wb_b(l2);
    l2_proc_refill_r(l2);
    l2_proc_read_serve(l2);
    l2_proc_write_wait_w(l2);
    l2_proc_write_serve(l2);
    l2_proc_wb_w(l2);
    l2_proc_wb_aw(l2);
    l2_proc_refill_ar(l2);
    l2_proc_idle(l2);

    AXI4_IF_DBG_CLOCK(l2, merged_);
}

void l2_free(l2_t *l2)
{
    mod_free(&l2->mod);
    axi_mux_free(&l2->axi_mux);
    AXI4_IF_FREE(l2, merged_);
    free(l2->tags);
    free(l2->data);
    free(l2->replace_ways);
    free(l2->valids);
    free(l2->dirtys);
}
