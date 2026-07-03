#include "l1.h"
#include <stdlib.h>
#include <string.h>
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define L1_WORD_SIZE 4u
#define L1_WORD_NUM (L1_LINE_SIZE / L1_WORD_SIZE)

static u32 l1_line_idx(const l1_t *l1, u32 set, u32 way)
{
    return set * l1->conf.way_num + way;
}

static u32 l1_line_addr(const l1_t *l1, u32 tag, u32 set)
{
    return ((tag * l1->set_num) + set) * L1_LINE_SIZE;
}

static u32 l1_req_line_addr(u32 addr)
{
    return addr & ~(L1_LINE_SIZE - 1u);
}

static u32 l1_req_set(const l1_t *l1, u32 addr)
{
    return (l1_req_line_addr(addr) / L1_LINE_SIZE) % l1->set_num;
}

static u32 l1_req_tag(const l1_t *l1, u32 addr)
{
    return (l1_req_line_addr(addr) / L1_LINE_SIZE) / l1->set_num;
}

static bool l1_bypass(const l1_t *l1, u32 addr)
{
    if (l1->conf.full_bypass) {
        return true;
    }

    for (u32 i = 0; i < L1_BYPASS_RANGE_NUM; i++) {
        if (l1->conf.bypass_sizes[i] == 0) {
            continue;
        }
        if (ADDR_IN(addr, l1->conf.bypass_bases[i], l1->conf.bypass_sizes[i])) {
            return true;
        }
    }
    return false;
}

static void l1_invalidate(l1_t *l1)
{
    for (u32 i = 0; i < l1->line_num; i++) {
        l1->valids[i] = false;
        l1->dirtys[i] = false;
    }
}

static u32 l1_req_size(const l1_t *l1)
{
    DBG_CHECK(l1->req.size <= BTI_REQ_SIZE_B4);
    return 1u << l1->req.size;
}

static u8 l1_read_byte(const l1_t *l1, u32 line_idx, u32 line_offset)
{
    u32 word = l1->data[line_idx * L1_WORD_NUM + line_offset / L1_WORD_SIZE];
    return (u8)(word >> ((line_offset % L1_WORD_SIZE) * 8u));
}

static void l1_write_byte(l1_t *l1, u32 line_idx, u32 line_offset, u8 data)
{
    u32 *word = &l1->data[line_idx * L1_WORD_NUM + line_offset / L1_WORD_SIZE];
    u32 shift = (line_offset % L1_WORD_SIZE) * 8u;
    *word = (*word & ~(0xffu << shift)) | ((u32)data << shift);
}

static void l1_send_rsp(l1_t *l1, bool ok, u32 data)
{
    bti_rsp_if_t rsp = {
        .trans_id = l1->req.trans_id,
        .data = data,
        .ok = ok
    };
    itf_write(l1->bti_rsp_mst, &rsp);
}

void l1_construct(l1_t *l1, const char *name, const l1_conf_t *conf)
{
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(l1->bti_req_slv);
    DBG_CHECK(l1->bti_rsp_mst);
    DBG_CHECK(l1->axi4_aw_mst);
    DBG_CHECK(l1->axi4_w_mst);
    DBG_CHECK(l1->axi4_b_slv);
    DBG_CHECK(l1->axi4_ar_mst);
    DBG_CHECK(l1->axi4_r_slv);
    DBG_CHECK(conf);
    DBG_CHECK(conf->way_num > 0);
    DBG_CHECK(conf->size >= L1_LINE_SIZE);
    DBG_CHECK((conf->size % (conf->way_num * L1_LINE_SIZE)) == 0);

    l1->conf = *conf;
    l1->set_num = conf->size / (conf->way_num * L1_LINE_SIZE);
    l1->line_num = l1->set_num * conf->way_num;
    l1->tags = malloc(sizeof(u32) * l1->line_num);
    l1->data = malloc(sizeof(u32) * l1->line_num * L1_WORD_NUM);
    l1->replace_ways = malloc(sizeof(u32) * l1->set_num);
    l1->valids = malloc(sizeof(bool) * l1->line_num);
    l1->dirtys = malloc(sizeof(bool) * l1->line_num);
    DBG_CHECK(l1->tags);
    DBG_CHECK(l1->data);
    DBG_CHECK(l1->replace_ways);
    DBG_CHECK(l1->valids);
    DBG_CHECK(l1->dirtys);

    dbg_vcd_add_sig("state", DBG_SIG_TYPE_REG, 4, &l1->state);
    dbg_vcd_add_sig("req_addr", DBG_SIG_TYPE_REG, 32, &l1->req.addr);
    dbg_vcd_add_sig("req_set", DBG_SIG_TYPE_REG, 32, &l1->req_set);
    dbg_vcd_add_sig("req_way", DBG_SIG_TYPE_REG, 32, &l1->req_way);
    dbg_vcd_add_sig("req_tag", DBG_SIG_TYPE_REG, 32, &l1->req_tag);
}

void l1_reset(l1_t *l1)
{
    l1->state = L1_STATE_IDLE;
    l1->beat_idx = 0;
    l1->op_ok = true;
    l1->req_byte_idx = 0;
    l1->req_data = 0;
    for (u32 i = 0; i < l1->line_num; i++) {
        l1->valids[i] = false;
        l1->dirtys[i] = false;
        l1->tags[i] = 0;
    }
    memset(l1->data, 0, sizeof(u32) * l1->line_num * L1_WORD_NUM);
    for (u32 i = 0; i < l1->set_num; i++) {
        l1->replace_ways[i] = 0;
    }
}

static bool l1_lookup(l1_t *l1, u32 *way, u32 *line_idx)
{
    for (u32 i = 0; i < l1->conf.way_num; i++) {
        u32 idx = l1_line_idx(l1, l1->req_set, i);
        if (l1->valids[idx] && l1->tags[idx] == l1->req_tag) {
            *way = i;
            *line_idx = idx;
            return true;
        }
    }
    return false;
}

static void l1_select_victim(l1_t *l1)
{
    for (u32 i = 0; i < l1->conf.way_num; i++) {
        u32 idx = l1_line_idx(l1, l1->req_set, i);
        if (!l1->valids[idx]) {
            l1->req_way = i;
            l1->req_line_idx = idx;
            return;
        }
    }

    l1->req_way = l1->replace_ways[l1->req_set];
    l1->req_line_idx = l1_line_idx(l1, l1->req_set, l1->req_way);
    l1->replace_ways[l1->req_set] = (l1->req_way + 1u) % l1->conf.way_num;
}

static void l1_setup_fragment(l1_t *l1)
{
    u32 addr = l1->req.addr + l1->req_byte_idx;
    l1->req_line_addr = l1_req_line_addr(addr);
    l1->req_set = l1_req_set(l1, addr);
    l1->req_tag = l1_req_tag(l1, addr);
}

static void l1_start_cached_fragment(l1_t *l1)
{
    u32 way;
    u32 line_idx;
    l1_setup_fragment(l1);
    if (l1_lookup(l1, &way, &line_idx)) {
        l1->req_way = way;
        l1->req_line_idx = line_idx;
        l1->state = L1_STATE_SERVE_MISS;
        return;
    }

    l1_select_victim(l1);
    if (l1->valids[l1->req_line_idx] && l1->dirtys[l1->req_line_idx]) {
        l1->wb_line_addr = l1_line_addr(l1, l1->tags[l1->req_line_idx], l1->req_set);
        l1->beat_idx = 0;
        l1->state = L1_STATE_WB_AW;
    } else {
        l1->beat_idx = 0;
        l1->state = L1_STATE_REFILL_AR;
    }
}

static bool l1_try_bypass_req(l1_t *l1)
{
    if (l1->req.cmd == BTI_REQ_CMD_READ) {
        if (itf_fifo_full(l1->axi4_ar_mst)) {
            return false;
        }
        itf_fifo_pop_front(l1->bti_req_slv);
        axi4_ar_if_t ar = {
            .id = 0,
            .addr = l1->req.addr,
            .len = 0,
            .size = (axi4_ar_size_t)l1->req.size,
            .burst = AXI4_AR_BURST_FIXED,
            .lock = false,
            .cache = 0,
            .prot = 0,
            .qos = 0,
            .user = 0
        };
        itf_write(l1->axi4_ar_mst, &ar);
        l1->state = L1_STATE_BYPASS_RD_WAIT;
        return true;
    }

    if (l1->conf.ro) {
        if (itf_fifo_full(l1->bti_rsp_mst)) {
            return false;
        }
        itf_fifo_pop_front(l1->bti_req_slv);
        l1_send_rsp(l1, false, 0);
        return true;
    }

    if (itf_fifo_full(l1->axi4_aw_mst) || itf_fifo_full(l1->axi4_w_mst)) {
        return false;
    }
    itf_fifo_pop_front(l1->bti_req_slv);
    axi4_aw_if_t aw = {
        .id = 0,
        .addr = l1->req.addr,
        .len = 0,
        .size = (axi4_aw_size_t)l1->req.size,
        .burst = AXI4_AW_BURST_FIXED,
        .lock = false,
        .cache = 0,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    axi4_w_if_t w = {
        .data = l1->req.data,
        .strb = (u8)(l1->req.strobe & 0xf),
        .last = true
    };
    itf_write(l1->axi4_aw_mst, &aw);
    itf_write(l1->axi4_w_mst, &w);
    l1->state = L1_STATE_BYPASS_WR_WAIT;
    return true;
}

static void l1_proc_idle(l1_t *l1)
{
    if (l1->state != L1_STATE_IDLE || itf_fifo_empty(l1->bti_req_slv)) {
        return;
    }

    itf_fifo_get_front(l1->bti_req_slv, &l1->req);
    l1->op_ok = true;
    l1->req_byte_idx = 0;
    l1->req_data = 0;

    if (l1_bypass(l1, l1->req.addr)) {
        (void)l1_try_bypass_req(l1);
        return;
    }

    if (l1->conf.ro && l1->req.cmd == BTI_REQ_CMD_WRITE) {
        if (itf_fifo_full(l1->bti_rsp_mst)) {
            return;
        }
        itf_fifo_pop_front(l1->bti_req_slv);
        l1_send_rsp(l1, false, 0);
        return;
    }

    itf_fifo_pop_front(l1->bti_req_slv);
    l1_start_cached_fragment(l1);
}

static void l1_proc_flush(l1_t *l1)
{
    if (!l1->flush_slv || itf_fifo_empty(l1->flush_slv)) {
        return;
    }

    l1_flush_if_t flush;
    itf_read(l1->flush_slv, &flush);
    l1_invalidate(l1);
}

static void l1_proc_bypass_rd_wait(l1_t *l1)
{
    if (l1->state != L1_STATE_BYPASS_RD_WAIT) {
        return;
    }
    if (itf_fifo_empty(l1->axi4_r_slv) || itf_fifo_full(l1->bti_rsp_mst)) {
        return;
    }

    axi4_r_if_t r;
    itf_read(l1->axi4_r_slv, &r);
    DBG_CHECK(r.last);
    l1_send_rsp(l1, r.resp == AXI4_R_RESP_OKAY, r.data);
    l1->state = L1_STATE_IDLE;
}

static void l1_proc_bypass_wr_wait(l1_t *l1)
{
    if (l1->state != L1_STATE_BYPASS_WR_WAIT) {
        return;
    }
    if (itf_fifo_empty(l1->axi4_b_slv) || itf_fifo_full(l1->bti_rsp_mst)) {
        return;
    }

    axi4_b_if_t b;
    itf_read(l1->axi4_b_slv, &b);
    l1_send_rsp(l1, b.resp == AXI4_B_RESP_OKAY, 0);
    l1->state = L1_STATE_IDLE;
}

static void l1_proc_wb_aw(l1_t *l1)
{
    if (l1->state != L1_STATE_WB_AW || itf_fifo_full(l1->axi4_aw_mst)) {
        return;
    }

    axi4_aw_if_t aw = {
        .id = 0,
        .addr = l1->wb_line_addr,
        .len = L1_WORD_NUM - 1u,
        .size = AXI4_AW_SIZE_B4,
        .burst = AXI4_AW_BURST_INCR,
        .lock = false,
        .cache = 0xf,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    itf_write(l1->axi4_aw_mst, &aw);
    l1->beat_idx = 0;
    l1->state = L1_STATE_WB_W;
}

static void l1_proc_wb_w(l1_t *l1)
{
    if (l1->state != L1_STATE_WB_W || itf_fifo_full(l1->axi4_w_mst)) {
        return;
    }

    axi4_w_if_t w = {
        .data = l1->data[l1->req_line_idx * L1_WORD_NUM + l1->beat_idx],
        .strb = 0xf,
        .last = l1->beat_idx == (L1_WORD_NUM - 1u)
    };
    itf_write(l1->axi4_w_mst, &w);
    if (w.last) {
        l1->state = L1_STATE_WB_B;
    } else {
        l1->beat_idx++;
    }
}

static void l1_proc_wb_b(l1_t *l1)
{
    if (l1->state != L1_STATE_WB_B || itf_fifo_empty(l1->axi4_b_slv)) {
        return;
    }

    axi4_b_if_t b;
    itf_read(l1->axi4_b_slv, &b);
    l1->op_ok = (b.resp == AXI4_B_RESP_OKAY);
    l1->dirtys[l1->req_line_idx] = false;
    l1->beat_idx = 0;
    l1->state = l1->op_ok ? L1_STATE_REFILL_AR : L1_STATE_SERVE_MISS;
}

static void l1_proc_refill_ar(l1_t *l1)
{
    if (l1->state != L1_STATE_REFILL_AR || itf_fifo_full(l1->axi4_ar_mst)) {
        return;
    }

    axi4_ar_if_t ar = {
        .id = 0,
        .addr = l1->req_line_addr,
        .len = L1_WORD_NUM - 1u,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .lock = false,
        .cache = 0xf,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    itf_write(l1->axi4_ar_mst, &ar);
    l1->beat_idx = 0;
    l1->op_ok = true;
    l1->state = L1_STATE_REFILL_R;
}

static void l1_proc_refill_r(l1_t *l1)
{
    if (l1->state != L1_STATE_REFILL_R || itf_fifo_empty(l1->axi4_r_slv)) {
        return;
    }

    axi4_r_if_t r;
    itf_read(l1->axi4_r_slv, &r);
    if (r.resp != AXI4_R_RESP_OKAY) {
        l1->op_ok = false;
    }
    l1->data[l1->req_line_idx * L1_WORD_NUM + l1->beat_idx] = r.data;

    bool last = r.last || l1->beat_idx == (L1_WORD_NUM - 1u);
    if (last) {
        l1->valids[l1->req_line_idx] = l1->op_ok;
        l1->dirtys[l1->req_line_idx] = false;
        l1->tags[l1->req_line_idx] = l1->req_tag;
        l1->state = L1_STATE_SERVE_MISS;
    } else {
        l1->beat_idx++;
    }
}

static void l1_proc_serve_miss(l1_t *l1)
{
    if (l1->state != L1_STATE_SERVE_MISS) {
        return;
    }

    if (!l1->op_ok) {
        if (itf_fifo_full(l1->bti_rsp_mst)) {
            return;
        }
        l1->valids[l1->req_line_idx] = false;
        l1_send_rsp(l1, false, 0);
        l1->state = L1_STATE_IDLE;
        return;
    }

    u32 req_size = l1_req_size(l1);
    u32 addr = l1->req.addr + l1->req_byte_idx;
    u32 line_offset = addr & (L1_LINE_SIZE - 1u);
    u32 byte_num = MIN(req_size - l1->req_byte_idx, L1_LINE_SIZE - line_offset);
    for (u32 i = 0; i < byte_num; i++) {
        u32 req_byte = l1->req_byte_idx + i;
        if (l1->req.cmd == BTI_REQ_CMD_WRITE) {
            if (l1->req.strobe & (1u << req_byte)) {
                l1_write_byte(l1, l1->req_line_idx, line_offset + i,
                    (u8)(l1->req.data >> (req_byte * 8u)));
                l1->dirtys[l1->req_line_idx] = true;
            }
        } else {
            l1->req_data |= (u32)l1_read_byte(l1, l1->req_line_idx, line_offset + i)
                << (req_byte * 8u);
        }
    }
    l1->req_byte_idx += byte_num;

    if (l1->req_byte_idx < req_size) {
        l1_start_cached_fragment(l1);
        return;
    }

    if (itf_fifo_full(l1->bti_rsp_mst)) {
        return;
    }
    l1_send_rsp(l1, true, l1->req.cmd == BTI_REQ_CMD_READ ? l1->req_data : 0);
    l1->state = L1_STATE_IDLE;
}

void l1_clock(l1_t *l1)
{
    if (l1->state == L1_STATE_IDLE) {
        l1_proc_flush(l1);
    }
    l1_proc_bypass_rd_wait(l1);
    l1_proc_bypass_wr_wait(l1);
    l1_proc_wb_b(l1);
    l1_proc_refill_r(l1);
    l1_proc_serve_miss(l1);
    l1_proc_wb_w(l1);
    l1_proc_wb_aw(l1);
    l1_proc_refill_ar(l1);
    l1_proc_idle(l1);
}

void l1_free(l1_t *l1)
{
    free(l1->tags);
    free(l1->data);
    free(l1->replace_ways);
    free(l1->valids);
    free(l1->dirtys);
}
