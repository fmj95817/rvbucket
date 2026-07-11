#include "l1.h"
#include <stdlib.h>
#include <string.h>
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
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

static u32 l1_req_size_from(const bti_req_if_t *req)
{
    DBG_CHECK(req->size <= BTI_REQ_SIZE_B4);
    return 1u << req->size;
}

static u32 l1_req_word_offset(const bti_req_if_t *req)
{
    return req->addr & (L1_WORD_SIZE - 1u);
}

static u32 l1_req_aligned_word_addr(const bti_req_if_t *req)
{
    return req->addr & ~(L1_WORD_SIZE - 1u);
}

static bool l1_req_cross_word(const bti_req_if_t *req)
{
    return l1_req_word_offset(req) + l1_req_size_from(req) > L1_WORD_SIZE;
}

static u32 l1_bypass_beat_addr(const l1_ost_ctx_t *ctx)
{
    return l1_req_aligned_word_addr(&ctx->req) +
        ctx->bypass_req_idx * L1_WORD_SIZE;
}

static u32 l1_bypass_first_read_data(u32 data, u32 offset)
{
    return data >> (offset * 8u);
}

static u32 l1_bypass_second_read_data(u32 data, u32 offset)
{
    return data << ((L1_WORD_SIZE - offset) * 8u);
}

static u32 l1_bypass_write_data(const l1_ost_ctx_t *ctx)
{
    u32 offset = l1_req_word_offset(&ctx->req);
    if (ctx->bypass_req_idx == 0) {
        return ctx->req.data << (offset * 8u);
    }
    return ctx->req.data >> ((L1_WORD_SIZE - offset) * 8u);
}

static u8 l1_bypass_write_strobe(const l1_ost_ctx_t *ctx)
{
    u32 offset = l1_req_word_offset(&ctx->req);
    if (ctx->bypass_req_idx == 0) {
        return (u8)((ctx->req.strobe << offset) & 0xfu);
    }
    return (u8)((ctx->req.strobe >> (L1_WORD_SIZE - offset)) & 0xfu);
}

static u32 l1_bypass_beat_num(const l1_ost_ctx_t *ctx)
{
    return ctx->bypass_split ? 2u : 1u;
}

static bool l1_bypass_slot_can_issue(const l1_ost_ctx_t *ctx)
{
    return !ctx->rsp_vld &&
        ctx->bypass_req_idx < l1_bypass_beat_num(ctx) &&
        ctx->bypass_req_idx == ctx->bypass_rsp_idx;
}

static bool l1_bypass_slot_wait_rsp(const l1_ost_ctx_t *ctx)
{
    return !ctx->rsp_vld && ctx->bypass_rsp_idx < ctx->bypass_req_idx;
}

static u32 l1_req_size(const l1_t *l1)
{
    return l1_req_size_from(&l1->req);
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

static void l1_send_rsp_by_id(l1_t *l1, u32 trans_id, bool ok, u32 data)
{
    bti_rsp_if_t rsp = {
        .trans_id = trans_id,
        .data = data,
        .ok = ok
    };
    itf_write(l1->bti_rsp_mst, &rsp);
}

void l1_construct(l1_t *l1, const char *parent, const char *name, const l1_conf_t *conf)
{
    mod_construct(&l1->mod, parent, name);
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
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);

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
    fifo_construct(&l1->req_fifo, sizeof(bti_req_if_t),
        conf->stg_fifo_depth);
    ostq_construct(&l1->ost, sizeof(l1_ost_ctx_t), conf->ost_depth);

    l1->perf_hit = dbg_pcm_reg_perf_cnt(l1->mod.hier_name, "hit");
    l1->perf_miss = dbg_pcm_reg_perf_cnt(l1->mod.hier_name, "miss");
    l1->perf_bypass = dbg_pcm_reg_perf_cnt(l1->mod.hier_name, "bypass");
    l1->perf_writeback = dbg_pcm_reg_perf_cnt(l1->mod.hier_name, "writeback");
    l1->perf_stg_full = dbg_pcm_reg_perf_cnt(l1->mod.hier_name, "stg_full");
    l1->perf_ost_full = dbg_pcm_reg_perf_cnt(l1->mod.hier_name, "ost_full");
    l1->perf_miss_busy = dbg_pcm_reg_perf_cnt(l1->mod.hier_name,
        "miss_busy");

    dbg_vcd_add_sig("state", DBG_SIG_TYPE_REG, 4, &l1->state);
    dbg_vcd_add_sig("req_addr", DBG_SIG_TYPE_REG, 32, &l1->req.addr);
    dbg_vcd_add_sig("req_set", DBG_SIG_TYPE_REG, 32, &l1->req_set);
    dbg_vcd_add_sig("req_way", DBG_SIG_TYPE_REG, 32, &l1->req_way);
    dbg_vcd_add_sig("req_tag", DBG_SIG_TYPE_REG, 32, &l1->req_tag);
    dbg_vcd_add_sig("req_fifo_num", DBG_SIG_TYPE_REG, 32, &l1->req_fifo.num);
    dbg_vcd_add_sig("ost_count", DBG_SIG_TYPE_REG, 32, &l1->ost.count);
}

void l1_reset(l1_t *l1)
{
    mod_reset(&l1->mod);
    l1->state = L1_STATE_IDLE;
    l1->active_slot = 0;
    l1->beat_idx = 0;
    l1->op_ok = true;
    l1->req_byte_idx = 0;
    l1->req_data = 0;
    fifo_reset(&l1->req_fifo);
    ostq_reset(&l1->ost);
    *l1->perf_hit = 0;
    *l1->perf_miss = 0;
    *l1->perf_bypass = 0;
    *l1->perf_writeback = 0;
    *l1->perf_stg_full = 0;
    *l1->perf_ost_full = 0;
    *l1->perf_miss_busy = 0;
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

static bool l1_lookup_addr(const l1_t *l1, u32 addr, u32 *way,
    u32 *line_idx)
{
    for (u32 i = 0; i < l1->conf.way_num; i++) {
        u32 set = l1_req_set(l1, addr);
        u32 tag = l1_req_tag(l1, addr);
        u32 idx = l1_line_idx(l1, set, i);
        if (l1->valids[idx] && l1->tags[idx] == tag) {
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
        (*l1->perf_hit)++;
        l1->req_way = way;
        l1->req_line_idx = line_idx;
        l1->state = L1_STATE_SERVE_MISS;
        return;
    }

    (*l1->perf_miss)++;
    l1_select_victim(l1);
    if (l1->valids[l1->req_line_idx] && l1->dirtys[l1->req_line_idx]) {
        (*l1->perf_writeback)++;
        l1->wb_line_addr = l1_line_addr(l1, l1->tags[l1->req_line_idx], l1->req_set);
        l1->beat_idx = 0;
        l1->state = L1_STATE_WB_AW;
    } else {
        l1->beat_idx = 0;
        l1->state = L1_STATE_REFILL_AR;
    }
}

static bool l1_alloc_ost(l1_t *l1, const bti_req_if_t *req,
    l1_ost_kind_t kind, u32 *slot)
{
    if (ostq_full(&l1->ost)) {
        (*l1->perf_ost_full)++;
        return false;
    }

    l1_ost_ctx_t ctx = {
        .trans_id = req->trans_id,
        .kind = kind,
        .req = *req,
        .bypass_split = (kind == L1_OST_BYPASS_RD ||
            kind == L1_OST_BYPASS_WR) && l1_req_cross_word(req),
        .bypass_req_idx = 0,
        .bypass_rsp_idx = 0,
        .rsp_vld = false,
        .rsp_delay_pend = false,
        .ok = false,
        .data = 0,
        .delay = 0
    };
    return ostq_alloc(&l1->ost, &ctx, slot);
}

static void l1_complete_slot(l1_t *l1, u32 slot, bool ok, u32 data)
{
    l1_ost_ctx_t ctx;
    ostq_read_slot(&l1->ost, slot, &ctx);
    ctx.ok = ok;
    ctx.data = data;
    ctx.rsp_vld = l1->conf.latency == 0;
    ctx.rsp_delay_pend = l1->conf.latency != 0;
    ctx.delay = 0;
    ostq_write_slot(&l1->ost, slot, &ctx);
}

static void l1_tick_rsp_delay(l1_t *l1)
{
    if (l1->conf.latency == 0) {
        return;
    }

    for (u32 i = 0; i < l1->ost.depth; i++) {
        if (!ostq_slot_valid(&l1->ost, i)) {
            continue;
        }

        l1_ost_ctx_t ctx;
        ostq_read_slot(&l1->ost, i, &ctx);
        if (!ctx.rsp_delay_pend) {
            continue;
        }

        ctx.delay++;
        if (ctx.delay >= l1->conf.latency) {
            ctx.rsp_delay_pend = false;
            ctx.rsp_vld = true;
        }
        ostq_write_slot(&l1->ost, i, &ctx);
    }
}

static bool l1_has_bypass_ost(l1_t *l1)
{
    for (u32 i = 0; i < l1->ost.depth; i++) {
        if (!ostq_slot_valid(&l1->ost, i)) {
            continue;
        }
        l1_ost_ctx_t ctx;
        ostq_read_slot(&l1->ost, i, &ctx);
        if (ctx.kind == L1_OST_BYPASS_RD ||
            ctx.kind == L1_OST_BYPASS_WR) {
            return true;
        }
    }
    return false;
}

static bool l1_active_line_conflict(const l1_t *l1, u32 line_idx)
{
    return l1->state != L1_STATE_IDLE && line_idx == l1->req_line_idx;
}

static bool l1_try_cached_hit_req(l1_t *l1, const bti_req_if_t *req,
    u32 *data)
{
    u32 req_size = l1_req_size_from(req);
    u32 req_byte_idx = 0;

    while (req_byte_idx < req_size) {
        u32 addr = req->addr + req_byte_idx;
        u32 way;
        u32 line_idx;
        if (!l1_lookup_addr(l1, addr, &way, &line_idx)) {
            return false;
        }
        (void)way;
        if (l1_active_line_conflict(l1, line_idx)) {
            return false;
        }

        u32 line_offset = addr & (L1_LINE_SIZE - 1u);
        req_byte_idx += MIN(req_size - req_byte_idx,
            L1_LINE_SIZE - line_offset);
    }

    *data = 0;
    req_byte_idx = 0;
    while (req_byte_idx < req_size) {
        u32 addr = req->addr + req_byte_idx;
        u32 way;
        u32 line_idx;
        bool hit = l1_lookup_addr(l1, addr, &way, &line_idx);
        DBG_CHECK(hit);
        (void)way;

        u32 line_offset = addr & (L1_LINE_SIZE - 1u);
        u32 byte_num = MIN(req_size - req_byte_idx,
            L1_LINE_SIZE - line_offset);
        for (u32 i = 0; i < byte_num; i++) {
            u32 req_byte = req_byte_idx + i;
            if (req->cmd == BTI_REQ_CMD_WRITE) {
                if (req->strobe & (1u << req_byte)) {
                    l1_write_byte(l1, line_idx, line_offset + i,
                        (u8)(req->data >> (req_byte * 8u)));
                    l1->dirtys[line_idx] = true;
                }
            } else {
                *data |= (u32)l1_read_byte(l1, line_idx, line_offset + i)
                    << (req_byte * 8u);
            }
        }
        req_byte_idx += byte_num;
    }

    return true;
}

static void l1_issue_bypass_read(l1_t *l1, u32 slot, l1_ost_ctx_t *ctx)
{
    DBG_CHECK(slot <= 0xffu);
    axi4_ar_if_t ar = {
        .id = (u8)slot,
        .addr = l1_bypass_beat_addr(ctx),
        .len = 0,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .lock = false,
        .cache = 0,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    itf_write(l1->axi4_ar_mst, &ar);
    ctx->bypass_req_idx++;
    ostq_write_slot(&l1->ost, slot, ctx);
}

static void l1_issue_bypass_write(l1_t *l1, u32 slot, l1_ost_ctx_t *ctx)
{
    DBG_CHECK(slot <= 0xffu);
    axi4_aw_if_t aw = {
        .id = (u8)slot,
        .addr = l1_bypass_beat_addr(ctx),
        .len = 0,
        .size = AXI4_AW_SIZE_B4,
        .burst = AXI4_AW_BURST_INCR,
        .lock = false,
        .cache = 0,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    axi4_w_if_t w = {
        .data = l1_bypass_write_data(ctx),
        .strb = l1_bypass_write_strobe(ctx),
        .last = true
    };
    itf_write(l1->axi4_aw_mst, &aw);
    itf_write(l1->axi4_w_mst, &w);
    ctx->bypass_req_idx++;
    ostq_write_slot(&l1->ost, slot, ctx);
}

static bool l1_try_bypass_req(l1_t *l1, const bti_req_if_t *req)
{
    if (l1->state != L1_STATE_IDLE) {
        return false;
    }

    u32 slot;

    if (req->cmd == BTI_REQ_CMD_READ) {
        if (itf_fifo_full(l1->axi4_ar_mst)) {
            return false;
        }
        if (!l1_alloc_ost(l1, req, L1_OST_BYPASS_RD, &slot)) {
            return false;
        }
        l1_ost_ctx_t ctx;
        ostq_read_slot(&l1->ost, slot, &ctx);
        l1_issue_bypass_read(l1, slot, &ctx);
        return true;
    }

    if (l1->conf.ro) {
        if (!l1_alloc_ost(l1, req, L1_OST_CACHED, &slot)) {
            return false;
        }
        l1_complete_slot(l1, slot, false, 0);
        return true;
    }

    if (itf_fifo_full(l1->axi4_aw_mst) || itf_fifo_full(l1->axi4_w_mst)) {
        return false;
    }
    if (!l1_alloc_ost(l1, req, L1_OST_BYPASS_WR, &slot)) {
        return false;
    }
    l1_ost_ctx_t ctx;
    ostq_read_slot(&l1->ost, slot, &ctx);
    l1_issue_bypass_write(l1, slot, &ctx);
    return true;
}

static void l1_capture_req(l1_t *l1)
{
    if (fifo_full(&l1->req_fifo)) {
        if (!itf_fifo_empty(l1->bti_req_slv)) {
            (*l1->perf_stg_full)++;
        }
        return;
    }

    if (itf_fifo_empty(l1->bti_req_slv)) {
        return;
    }

    bti_req_if_t req;
    itf_read(l1->bti_req_slv, &req);
    fifo_push(&l1->req_fifo, &req);
}

static void l1_proc_accept_req(l1_t *l1)
{
    if (fifo_empty(&l1->req_fifo)) {
        return;
    }
    if (ostq_full(&l1->ost)) {
        (*l1->perf_ost_full)++;
        return;
    }

    bti_req_if_t req;
    fifo_get_front(&l1->req_fifo, &req);

    if (l1_bypass(l1, req.addr)) {
        (*l1->perf_bypass)++;
        if (l1_try_bypass_req(l1, &req)) {
            fifo_pop(&l1->req_fifo, &req);
        }
        return;
    }

    if (l1->conf.ro && req.cmd == BTI_REQ_CMD_WRITE) {
        u32 slot;
        if (!l1_alloc_ost(l1, &req, L1_OST_CACHED, &slot)) {
            return;
        }
        fifo_pop(&l1->req_fifo, &req);
        l1_complete_slot(l1, slot, false, 0);
        return;
    }

    u32 hit_data;
    if (l1_try_cached_hit_req(l1, &req, &hit_data)) {
        (*l1->perf_hit)++;
        u32 slot;
        if (!l1_alloc_ost(l1, &req, L1_OST_CACHED, &slot)) {
            return;
        }
        fifo_pop(&l1->req_fifo, &req);
        l1_complete_slot(l1, slot, true,
            req.cmd == BTI_REQ_CMD_READ ? hit_data : 0);
        return;
    }

    if (l1->state != L1_STATE_IDLE || l1_has_bypass_ost(l1)) {
        (*l1->perf_miss_busy)++;
        return;
    }

    u32 slot;
    if (!l1_alloc_ost(l1, &req, L1_OST_CACHED, &slot)) {
        return;
    }
    fifo_pop(&l1->req_fifo, &req);
    l1->active_slot = slot;
    l1->req = req;
    l1->op_ok = true;
    l1->req_byte_idx = 0;
    l1->req_data = 0;
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

static bool l1_find_bypass_issue_slot(l1_t *l1, l1_ost_kind_t kind,
    l1_ost_ctx_t *ctx, u32 *slot)
{
    for (u32 i = 0; i < l1->ost.depth; i++) {
        u32 idx = (l1->ost.rptr + i) % l1->ost.depth;
        if (!ostq_slot_valid(&l1->ost, idx)) {
            continue;
        }

        l1_ost_ctx_t cur;
        ostq_read_slot(&l1->ost, idx, &cur);
        if (cur.kind != kind || !l1_bypass_slot_can_issue(&cur)) {
            continue;
        }

        if (ctx != NULL) {
            *ctx = cur;
        }
        if (slot != NULL) {
            *slot = idx;
        }
        return true;
    }
    return false;
}

static void l1_mark_bypass_rsp(l1_t *l1, u32 slot, l1_ost_ctx_t *ctx)
{
    if (ctx->bypass_rsp_idx == l1_bypass_beat_num(ctx)) {
        ctx->rsp_vld = l1->conf.latency == 0;
        ctx->rsp_delay_pend = l1->conf.latency != 0;
        ctx->delay = 0;
    }
    ostq_write_slot(&l1->ost, slot, ctx);
}

static void l1_proc_bypass_rd_wait(l1_t *l1)
{
    if (itf_fifo_empty(l1->axi4_r_slv)) {
        return;
    }

    axi4_r_if_t r;
    itf_fifo_get_front(l1->axi4_r_slv, &r);
    u32 slot = r.id;
    if (slot >= l1->ost.depth || !ostq_slot_valid(&l1->ost, slot)) {
        return;
    }

    l1_ost_ctx_t ctx;
    ostq_read_slot(&l1->ost, slot, &ctx);
    if (ctx.kind != L1_OST_BYPASS_RD || !l1_bypass_slot_wait_rsp(&ctx)) {
        return;
    }

    itf_fifo_pop_front(l1->axi4_r_slv);
    DBG_CHECK(r.last);
    bool resp_ok = r.resp == AXI4_R_RESP_OKAY;
    ctx.ok = ctx.bypass_rsp_idx == 0 ? resp_ok : ctx.ok && resp_ok;
    if (ctx.bypass_rsp_idx == 0) {
        ctx.data = l1_bypass_first_read_data(r.data,
            l1_req_word_offset(&ctx.req));
    } else {
        ctx.data |= l1_bypass_second_read_data(r.data,
            l1_req_word_offset(&ctx.req));
    }
    ctx.bypass_rsp_idx++;
    l1_mark_bypass_rsp(l1, slot, &ctx);
}

static void l1_proc_bypass_wr_wait(l1_t *l1)
{
    if (itf_fifo_empty(l1->axi4_b_slv)) {
        return;
    }

    axi4_b_if_t b;
    itf_fifo_get_front(l1->axi4_b_slv, &b);
    u32 slot = b.id;
    if (slot >= l1->ost.depth || !ostq_slot_valid(&l1->ost, slot)) {
        return;
    }

    l1_ost_ctx_t ctx;
    ostq_read_slot(&l1->ost, slot, &ctx);
    if (ctx.kind != L1_OST_BYPASS_WR || !l1_bypass_slot_wait_rsp(&ctx)) {
        return;
    }

    itf_fifo_pop_front(l1->axi4_b_slv);
    bool resp_ok = b.resp == AXI4_B_RESP_OKAY;
    ctx.ok = ctx.bypass_rsp_idx == 0 ? resp_ok : ctx.ok && resp_ok;
    ctx.data = 0;
    ctx.bypass_rsp_idx++;
    l1_mark_bypass_rsp(l1, slot, &ctx);
}

static void l1_proc_bypass_rd_issue(l1_t *l1)
{
    if (itf_fifo_full(l1->axi4_ar_mst)) {
        return;
    }

    l1_ost_ctx_t ctx;
    u32 slot;
    bool found = l1_find_bypass_issue_slot(l1, L1_OST_BYPASS_RD, &ctx, &slot);
    if (!found) {
        return;
    }
    l1_issue_bypass_read(l1, slot, &ctx);
}

static void l1_proc_bypass_wr_issue(l1_t *l1)
{
    if (itf_fifo_full(l1->axi4_aw_mst) || itf_fifo_full(l1->axi4_w_mst)) {
        return;
    }

    l1_ost_ctx_t ctx;
    u32 slot;
    bool found = l1_find_bypass_issue_slot(l1, L1_OST_BYPASS_WR, &ctx, &slot);
    if (!found) {
        return;
    }
    l1_issue_bypass_write(l1, slot, &ctx);
}

static void l1_send_ost_rsp(l1_t *l1)
{
    if (itf_fifo_full(l1->bti_rsp_mst)) {
        return;
    }

    l1_ost_ctx_t ctx;
    bool found = ostq_peek_head(&l1->ost, &ctx, NULL);
    if (!found || !ctx.rsp_vld) {
        return;
    }

    l1_send_rsp_by_id(l1, ctx.trans_id, ctx.ok, ctx.data);
    ostq_free_head(&l1->ost);
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
        l1->valids[l1->req_line_idx] = false;
        l1_complete_slot(l1, l1->active_slot, false, 0);
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

    l1_complete_slot(l1, l1->active_slot, true,
        l1->req.cmd == BTI_REQ_CMD_READ ? l1->req_data : 0);
    l1->state = L1_STATE_IDLE;
}

void l1_clock(l1_t *l1)
{
    mod_clock(&l1->mod);
    ostq_clock(&l1->ost);
    l1_tick_rsp_delay(l1);
    l1_capture_req(l1);
    if (l1->state == L1_STATE_IDLE) {
        l1_proc_flush(l1);
    }
    l1_proc_bypass_rd_wait(l1);
    l1_proc_bypass_wr_wait(l1);
    l1_proc_bypass_rd_issue(l1);
    l1_proc_bypass_wr_issue(l1);
    l1_proc_wb_b(l1);
    l1_proc_refill_r(l1);
    l1_proc_serve_miss(l1);
    l1_send_ost_rsp(l1);
    l1_proc_wb_w(l1);
    l1_proc_wb_aw(l1);
    l1_proc_refill_ar(l1);
    l1_proc_accept_req(l1);
}

void l1_free(l1_t *l1)
{
    mod_free(&l1->mod);
    fifo_free(&l1->req_fifo);
    ostq_free(&l1->ost);
    free(l1->tags);
    free(l1->data);
    free(l1->replace_ways);
    free(l1->valids);
    free(l1->dirtys);
}
