#include "axi2bti.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

static u32 axi2bti_next_addr(u32 addr, u8 size, u8 burst)
{
    switch (burst) {
    case 0: /* FIXED */
        return addr;
    case 1: /* INCR */
        return addr + (1u << size);
    default: /* WRAP not supported */
        return addr + (1u << size);
    }
}

static bool axi2bti_bti_id_active(axi2bti_t *br, u16 trans_id)
{
    for (u32 i = 0; i < br->beat_ost.depth; i++) {
        ostk_entry_t *entry = &br->beat_ost.entries[i];
        if (entry->vld && !entry->free_pending && entry->key == trans_id) {
            return true;
        }
    }
    return false;
}

static u16 axi2bti_alloc_bti_trans_id(axi2bti_t *br)
{
    for (u32 i = 0; i < 0xffffu; i++) {
        u16 trans_id = br->next_bti_trans_id++;
        if (br->next_bti_trans_id == 0) {
            br->next_bti_trans_id = 1;
        }
        if (trans_id == 0) {
            continue;
        }
        if (!axi2bti_bti_id_active(br, trans_id)) {
            return trans_id;
        }
    }

    DBG_CHECK(false);
    return 0;
}

static bool axi2bti_find_read_to_issue(axi2bti_t *br, u32 *slot)
{
    bool found = false;
    u32 best_slot = 0;
    u64 best_seq = 0;

    for (u32 i = 0; i < br->rd_ost.depth; i++) {
        ostk_entry_t *entry = &br->rd_ost.entries[i];
        if (!entry->vld || entry->free_pending) {
            continue;
        }

        axi2bti_rd_ctx_t *ctx = (axi2bti_rd_ctx_t *)entry->ctx;
        if (ctx->bti_rsp_pending || ctx->rsp_valid ||
            ctx->beat_idx > ctx->axlen) {
            continue;
        }

        if (!found || entry->issue_seq < best_seq) {
            found = true;
            best_slot = i;
            best_seq = entry->issue_seq;
        }
    }

    if (found) {
        *slot = best_slot;
    }
    return found;
}

static bool axi2bti_is_oldest_read_id_slot(axi2bti_t *br, u32 slot)
{
    ostk_entry_t *entry = &br->rd_ost.entries[slot];
    axi2bti_rd_ctx_t oldest_ctx;
    u32 oldest_slot;
    bool found = ostk_peek_key(&br->rd_ost, entry->key, &oldest_ctx,
        &oldest_slot);
    DBG_CHECK(found);
    return oldest_slot == slot;
}

static bool axi2bti_find_read_to_output(axi2bti_t *br, u32 *slot)
{
    bool found = false;
    u32 best_slot = 0;
    u64 best_seq = 0;

    for (u32 i = 0; i < br->rd_ost.depth; i++) {
        ostk_entry_t *entry = &br->rd_ost.entries[i];
        if (!entry->vld || entry->free_pending) {
            continue;
        }

        axi2bti_rd_ctx_t *ctx = (axi2bti_rd_ctx_t *)entry->ctx;
        if (!ctx->rsp_valid) {
            continue;
        }
        if (!axi2bti_is_oldest_read_id_slot(br, i)) {
            continue;
        }

        if (!found || entry->issue_seq < best_seq) {
            found = true;
            best_slot = i;
            best_seq = entry->issue_seq;
        }
    }

    if (found) {
        *slot = best_slot;
    }
    return found;
}

void axi2bti_construct(axi2bti_t *br, const char *parent, const char *name,
    const axi2bti_conf_t *conf)
{
    mod_construct(&br->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);

    DBG_CHECK(br->axi4_ar_slv);
    DBG_CHECK(br->axi4_r_mst);
    DBG_CHECK(br->axi4_aw_slv);
    DBG_CHECK(br->axi4_w_slv);
    DBG_CHECK(br->axi4_b_mst);
    DBG_CHECK(br->bti_req_mst);
    DBG_CHECK(br->bti_rsp_slv);

    fifo_construct(&br->ar_fifo, sizeof(axi4_ar_if_t),
        conf->stg_fifo_depth);
    fifo_construct(&br->aw_fifo, sizeof(axi4_aw_if_t),
        conf->stg_fifo_depth);
    fifo_construct(&br->w_fifo, sizeof(axi4_w_if_t),
        conf->stg_fifo_depth);
    ostk_construct(&br->rd_ost, sizeof(axi2bti_rd_ctx_t),
        conf->ost_depth);
    ostq_construct(&br->wr_ost, sizeof(axi2bti_wr_ctx_t),
        conf->ost_depth);
    ostk_construct(&br->beat_ost, sizeof(axi2bti_beat_ctx_t),
        conf->ost_depth);

    br->perf_stg_ar_full = dbg_pcm_reg_perf_cnt(br->mod.hier_name,
        "stg_ar_full");
    br->perf_stg_aw_full = dbg_pcm_reg_perf_cnt(br->mod.hier_name,
        "stg_aw_full");
    br->perf_stg_w_full = dbg_pcm_reg_perf_cnt(br->mod.hier_name,
        "stg_w_full");
}

void axi2bti_reset(axi2bti_t *br)
{
    mod_reset(&br->mod);
    fifo_reset(&br->ar_fifo);
    fifo_reset(&br->aw_fifo);
    fifo_reset(&br->w_fifo);
    ostk_reset(&br->rd_ost);
    ostq_reset(&br->wr_ost);
    ostk_reset(&br->beat_ost);
    br->next_bti_trans_id = 1;
    br->issue_wr_prio = false;
    *br->perf_stg_ar_full = 0;
    *br->perf_stg_aw_full = 0;
    *br->perf_stg_w_full = 0;
}

static void axi2bti_capture_ar(axi2bti_t *br)
{
    if (fifo_full(&br->ar_fifo)) {
        if (!itf_fifo_empty(br->axi4_ar_slv)) {
            (*br->perf_stg_ar_full)++;
        }
        return;
    }

    if (itf_fifo_empty(br->axi4_ar_slv)) {
        return;
    }

    axi4_ar_if_t ar;
    itf_read(br->axi4_ar_slv, &ar);
    fifo_push(&br->ar_fifo, &ar);
}

static void axi2bti_capture_aw(axi2bti_t *br)
{
    if (fifo_full(&br->aw_fifo)) {
        if (!itf_fifo_empty(br->axi4_aw_slv)) {
            (*br->perf_stg_aw_full)++;
        }
        return;
    }

    if (itf_fifo_empty(br->axi4_aw_slv)) {
        return;
    }

    axi4_aw_if_t aw;
    itf_read(br->axi4_aw_slv, &aw);
    fifo_push(&br->aw_fifo, &aw);
}

static void axi2bti_capture_w(axi2bti_t *br)
{
    if (fifo_full(&br->w_fifo)) {
        if (!itf_fifo_empty(br->axi4_w_slv)) {
            (*br->perf_stg_w_full)++;
        }
        return;
    }

    if (itf_fifo_empty(br->axi4_w_slv)) {
        return;
    }

    axi4_w_if_t w;
    itf_read(br->axi4_w_slv, &w);
    fifo_push(&br->w_fifo, &w);
}

static void axi2bti_proc_ar(axi2bti_t *br)
{
    if (fifo_empty(&br->ar_fifo)) {
        return;
    }

    axi4_ar_if_t ar;
    fifo_get_front(&br->ar_fifo, &ar);

    if (ar.burst == AXI4_AR_BURST_WRAP) {
        if (itf_fifo_full(br->axi4_r_mst)) {
            return;
        }

        fifo_pop(&br->ar_fifo, &ar);
        axi4_r_if_t r = {
            .id = ar.id,
            .data = 0,
            .resp = AXI4_R_RESP_SLVERR,
            .last = true
        };
        itf_write(br->axi4_r_mst, &r);
        return;
    }

    if (ostk_full(&br->rd_ost)) {
        return;
    }

    fifo_pop(&br->ar_fifo, &ar);

    axi2bti_rd_ctx_t ctx = {
        .axid = ar.id,
        .axaddr = ar.addr,
        .axlen = ar.len,
        .axsize = (u8)ar.size,
        .axburst = (u8)ar.burst,
        .beat_idx = 0,
        .bti_rsp_pending = false,
        .rsp_valid = false,
        .rsp_ok = false,
        .rsp_data = 0
    };
    bool alloc_ok = ostk_alloc(&br->rd_ost, ar.id, &ctx, NULL);
    DBG_CHECK(alloc_ok);
}

static void axi2bti_proc_aw(axi2bti_t *br)
{
    if (fifo_empty(&br->aw_fifo)) {
        return;
    }

    axi4_aw_if_t aw;
    fifo_get_front(&br->aw_fifo, &aw);

    if (aw.burst == AXI4_AW_BURST_WRAP) {
        if (itf_fifo_full(br->axi4_b_mst)) {
            return;
        }

        fifo_pop(&br->aw_fifo, &aw);
        axi4_b_if_t b = { .id = aw.id, .resp = AXI4_B_RESP_SLVERR };
        itf_write(br->axi4_b_mst, &b);
        return;
    }

    if (ostq_full(&br->wr_ost)) {
        return;
    }

    fifo_pop(&br->aw_fifo, &aw);

    axi2bti_wr_ctx_t ctx = {
        .axid = aw.id,
        .axaddr = aw.addr,
        .axlen = aw.len,
        .axsize = (u8)aw.size,
        .axburst = (u8)aw.burst,
        .beat_idx = 0,
        .bti_rsp_pending = false,
        .pending_last = false,
        .b_valid = false,
        .b_ok = false
    };
    bool alloc_ok = ostq_alloc(&br->wr_ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);
}

static void axi2bti_proc_bti_rsp(axi2bti_t *br)
{
    if (itf_fifo_empty(br->bti_rsp_slv)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_fifo_get_front(br->bti_rsp_slv, &rsp);

    axi2bti_beat_ctx_t beat_ctx;
    u32 beat_slot;
    bool found = ostk_peek_key(&br->beat_ost, rsp.trans_id, &beat_ctx,
        &beat_slot);
    DBG_CHECK(found);

    itf_fifo_pop_front(br->bti_rsp_slv);

    if (beat_ctx.kind == AXI2BTI_BEAT_READ) {
        DBG_CHECK(ostk_slot_valid(&br->rd_ost, beat_ctx.slot));

        axi2bti_rd_ctx_t rd_ctx;
        ostk_read_slot(&br->rd_ost, beat_ctx.slot, &rd_ctx);
        DBG_CHECK(rd_ctx.bti_rsp_pending);
        DBG_CHECK(!rd_ctx.rsp_valid);

        rd_ctx.bti_rsp_pending = false;
        rd_ctx.rsp_valid = true;
        rd_ctx.rsp_ok = rsp.ok;
        rd_ctx.rsp_data = rsp.data;
        ostk_write_slot(&br->rd_ost, beat_ctx.slot, &rd_ctx);
    } else {
        DBG_CHECK(ostq_slot_valid(&br->wr_ost, beat_ctx.slot));

        axi2bti_wr_ctx_t wr_ctx;
        ostq_read_slot(&br->wr_ost, beat_ctx.slot, &wr_ctx);
        DBG_CHECK(wr_ctx.bti_rsp_pending);
        DBG_CHECK(!wr_ctx.b_valid);

        wr_ctx.bti_rsp_pending = false;
        if (!rsp.ok || wr_ctx.pending_last) {
            wr_ctx.b_valid = true;
            wr_ctx.b_ok = rsp.ok;
        } else {
            wr_ctx.axaddr = axi2bti_next_addr(wr_ctx.axaddr, wr_ctx.axsize,
                wr_ctx.axburst);
            wr_ctx.beat_idx++;
        }
        wr_ctx.pending_last = false;
        ostq_write_slot(&br->wr_ost, beat_ctx.slot, &wr_ctx);
    }

    ostk_free_slot(&br->beat_ost, beat_slot);
}

static void axi2bti_proc_r(axi2bti_t *br)
{
    if (itf_fifo_full(br->axi4_r_mst)) {
        return;
    }

    u32 slot;
    if (!axi2bti_find_read_to_output(br, &slot)) {
        return;
    }

    axi2bti_rd_ctx_t ctx;
    ostk_read_slot(&br->rd_ost, slot, &ctx);

    bool last = (ctx.beat_idx == ctx.axlen) || !ctx.rsp_ok;
    axi4_r_if_t r = {
        .id = ctx.axid,
        .data = ctx.rsp_ok ? ctx.rsp_data : 0,
        .resp = ctx.rsp_ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR,
        .last = last
    };
    itf_write(br->axi4_r_mst, &r);

    if (last) {
        ostk_free_slot(&br->rd_ost, slot);
    } else {
        ctx.rsp_valid = false;
        ctx.rsp_ok = false;
        ctx.rsp_data = 0;
        ctx.axaddr = axi2bti_next_addr(ctx.axaddr, ctx.axsize, ctx.axburst);
        ctx.beat_idx++;
        ostk_write_slot(&br->rd_ost, slot, &ctx);
    }
}

static void axi2bti_proc_b(axi2bti_t *br)
{
    if (itf_fifo_full(br->axi4_b_mst)) {
        return;
    }

    axi2bti_wr_ctx_t ctx;
    u32 slot;
    if (!ostq_peek_head(&br->wr_ost, &ctx, &slot)) {
        return;
    }
    if (!ctx.b_valid) {
        return;
    }

    axi4_b_if_t b = {
        .id = ctx.axid,
        .resp = ctx.b_ok ? AXI4_B_RESP_OKAY : AXI4_B_RESP_SLVERR
    };
    itf_write(br->axi4_b_mst, &b);
    ostq_free_head(&br->wr_ost);
}

static bool axi2bti_try_issue_read_req(axi2bti_t *br)
{
    if (itf_fifo_full(br->bti_req_mst) || ostk_full(&br->beat_ost)) {
        return false;
    }

    u32 rd_slot;
    if (!axi2bti_find_read_to_issue(br, &rd_slot)) {
        return false;
    }

    axi2bti_rd_ctx_t rd_ctx;
    ostk_read_slot(&br->rd_ost, rd_slot, &rd_ctx);

    u16 trans_id = axi2bti_alloc_bti_trans_id(br);
    axi2bti_beat_ctx_t beat_ctx = {
        .kind = AXI2BTI_BEAT_READ,
        .slot = rd_slot
    };
    bool alloc_ok = ostk_alloc(&br->beat_ost, trans_id, &beat_ctx, NULL);
    DBG_CHECK(alloc_ok);

    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_READ,
        .addr = rd_ctx.axaddr,
        .size = (bti_req_size_t)rd_ctx.axsize,
        .data = 0,
        .strobe = 0
    };
    itf_write(br->bti_req_mst, &req);

    rd_ctx.bti_rsp_pending = true;
    ostk_write_slot(&br->rd_ost, rd_slot, &rd_ctx);
    return true;
}

static bool axi2bti_try_issue_write_req(axi2bti_t *br)
{
    if (itf_fifo_full(br->bti_req_mst) || ostk_full(&br->beat_ost)) {
        return false;
    }

    axi2bti_wr_ctx_t wr_ctx;
    u32 wr_slot;
    if (!ostq_peek_head(&br->wr_ost, &wr_ctx, &wr_slot)) {
        return false;
    }
    if (wr_ctx.bti_rsp_pending || wr_ctx.b_valid) {
        return false;
    }
    if (fifo_empty(&br->w_fifo)) {
        return false;
    }

    axi4_w_if_t w;
    fifo_get_front(&br->w_fifo, &w);
    bool last = w.last || (wr_ctx.beat_idx == wr_ctx.axlen);

    u16 trans_id = axi2bti_alloc_bti_trans_id(br);
    axi2bti_beat_ctx_t beat_ctx = {
        .kind = AXI2BTI_BEAT_WRITE,
        .slot = wr_slot
    };
    bool alloc_ok = ostk_alloc(&br->beat_ost, trans_id, &beat_ctx, NULL);
    DBG_CHECK(alloc_ok);

    fifo_pop(&br->w_fifo, &w);

    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_WRITE,
        .addr = wr_ctx.axaddr,
        .size = (bti_req_size_t)wr_ctx.axsize,
        .data = w.data,
        .strobe = w.strb
    };
    itf_write(br->bti_req_mst, &req);

    wr_ctx.bti_rsp_pending = true;
    wr_ctx.pending_last = last;
    ostq_write_slot(&br->wr_ost, wr_slot, &wr_ctx);
    return true;
}

static void axi2bti_proc_issue_req(axi2bti_t *br)
{
    if (br->issue_wr_prio) {
        if (axi2bti_try_issue_write_req(br)) {
            br->issue_wr_prio = false;
            return;
        }
        if (axi2bti_try_issue_read_req(br)) {
            br->issue_wr_prio = true;
        }
    } else {
        if (axi2bti_try_issue_read_req(br)) {
            br->issue_wr_prio = true;
            return;
        }
        if (axi2bti_try_issue_write_req(br)) {
            br->issue_wr_prio = false;
        }
    }
}

void axi2bti_clock(axi2bti_t *br)
{
    mod_clock(&br->mod);
    ostk_clock(&br->rd_ost);
    ostq_clock(&br->wr_ost);
    ostk_clock(&br->beat_ost);
    axi2bti_capture_ar(br);
    axi2bti_capture_aw(br);
    axi2bti_capture_w(br);
    axi2bti_proc_bti_rsp(br);
    axi2bti_proc_r(br);
    axi2bti_proc_b(br);
    axi2bti_proc_ar(br);
    axi2bti_proc_aw(br);
    axi2bti_proc_issue_req(br);
}

void axi2bti_free(axi2bti_t *br)
{
    mod_free(&br->mod);
    fifo_free(&br->ar_fifo);
    fifo_free(&br->aw_fifo);
    fifo_free(&br->w_fifo);
    ostk_free(&br->rd_ost);
    ostq_free(&br->wr_ost);
    ostk_free(&br->beat_ost);
}
