#include "axi_demux.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

void axi_demux_construct(axi_demux_t *axi_demux, const char *parent, const char *name,
    const axi_demux_conf_t *conf)
{
    mod_construct(&axi_demux->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(conf);
    DBG_CHECK(conf->gst_num <= AXI_DEMUX_GST_NUM_MAX);
    DBG_CHECK(conf->gst_bases);
    DBG_CHECK(conf->gst_sizes);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);
    axi_demux->gst_num = conf->gst_num;

    for (u32 i = 0; i < conf->gst_num; i++) {
        axi_demux->gst_bases[i] = conf->gst_bases[i];
        axi_demux->gst_sizes[i] = conf->gst_sizes[i];
    }

    fifo_construct(&axi_demux->ar_fifo, sizeof(axi4_ar_if_t),
        conf->stg_fifo_depth);
    fifo_construct(&axi_demux->aw_fifo, sizeof(axi4_aw_if_t),
        conf->stg_fifo_depth);
    fifo_construct(&axi_demux->w_fifo, sizeof(axi4_w_if_t),
        conf->stg_fifo_depth);
    ostk_construct(&axi_demux->rd_ost, sizeof(axi_demux_rd_ctx_t),
        conf->ost_depth);
    ostq_construct(&axi_demux->wr_data_ost, sizeof(axi_demux_wr_data_ctx_t),
        conf->ost_depth);
    ostk_construct(&axi_demux->wr_rsp_ost, sizeof(axi_demux_wr_rsp_ctx_t),
        conf->ost_depth);

    axi_demux->perf_stg_ar_full = dbg_pcm_reg_perf_cnt(
        axi_demux->mod.hier_name, "stg_ar_full");
    axi_demux->perf_stg_aw_full = dbg_pcm_reg_perf_cnt(
        axi_demux->mod.hier_name, "stg_aw_full");
    axi_demux->perf_stg_w_full = dbg_pcm_reg_perf_cnt(
        axi_demux->mod.hier_name, "stg_w_full");
}

void axi_demux_reset(axi_demux_t *axi_demux)
{
    mod_reset(&axi_demux->mod);
    axi_demux->rd_rsp_rr_idx = 0;
    axi_demux->wr_rsp_rr_idx = 0;
    fifo_reset(&axi_demux->ar_fifo);
    fifo_reset(&axi_demux->aw_fifo);
    fifo_reset(&axi_demux->w_fifo);
    ostk_reset(&axi_demux->rd_ost);
    ostq_reset(&axi_demux->wr_data_ost);
    ostk_reset(&axi_demux->wr_rsp_ost);
    *axi_demux->perf_stg_ar_full = 0;
    *axi_demux->perf_stg_aw_full = 0;
    *axi_demux->perf_stg_w_full = 0;
}

static bool axi_demux_decode(axi_demux_t *axi_demux, u32 addr, u32 *gst_idx)
{
    for (u32 i = 0; i < axi_demux->gst_num; i++) {
        if (ADDR_IN(addr, axi_demux->gst_bases[i], axi_demux->gst_sizes[i])) {
            *gst_idx = i;
            return true;
        }
    }
    return false;
}

static void axi_demux_capture_ar(axi_demux_t *axi_demux)
{
    if (fifo_full(&axi_demux->ar_fifo)) {
        if (!itf_fifo_empty(axi_demux->host_axi4_ar_slv)) {
            (*axi_demux->perf_stg_ar_full)++;
        }
        return;
    }

    if (itf_fifo_empty(axi_demux->host_axi4_ar_slv)) {
        return;
    }

    axi4_ar_if_t ar;
    itf_read(axi_demux->host_axi4_ar_slv, &ar);
    fifo_push(&axi_demux->ar_fifo, &ar);
}

static void axi_demux_capture_aw(axi_demux_t *axi_demux)
{
    if (fifo_full(&axi_demux->aw_fifo)) {
        if (!itf_fifo_empty(axi_demux->host_axi4_aw_slv)) {
            (*axi_demux->perf_stg_aw_full)++;
        }
        return;
    }

    if (itf_fifo_empty(axi_demux->host_axi4_aw_slv)) {
        return;
    }

    axi4_aw_if_t aw;
    itf_read(axi_demux->host_axi4_aw_slv, &aw);
    fifo_push(&axi_demux->aw_fifo, &aw);
}

static void axi_demux_capture_w(axi_demux_t *axi_demux)
{
    if (fifo_full(&axi_demux->w_fifo)) {
        if (!itf_fifo_empty(axi_demux->host_axi4_w_slv)) {
            (*axi_demux->perf_stg_w_full)++;
        }
        return;
    }

    if (itf_fifo_empty(axi_demux->host_axi4_w_slv)) {
        return;
    }

    axi4_w_if_t w;
    itf_read(axi_demux->host_axi4_w_slv, &w);
    fifo_push(&axi_demux->w_fifo, &w);
}

static void axi_demux_proc_ar(axi_demux_t *axi_demux)
{
    if (fifo_empty(&axi_demux->ar_fifo)) {
        return;
    }

    axi4_ar_if_t ar;
    fifo_get_front(&axi_demux->ar_fifo, &ar);

    u32 gst_idx;
    if (!axi_demux_decode(axi_demux, ar.addr, &gst_idx)) {
        if (itf_fifo_full(axi_demux->host_axi4_r_mst)) {
            return;
        }

        fifo_pop(&axi_demux->ar_fifo, &ar);
        axi4_r_if_t r = {
            .id = ar.id,
            .data = 0,
            .resp = AXI4_R_RESP_DECERR,
            .last = true
        };
        itf_write(axi_demux->host_axi4_r_mst, &r);
        return;
    }

    if (ostk_full(&axi_demux->rd_ost)) {
        return;
    }

    itf_t *gst_ar = axi_demux->gst_axi4_ar_msts[gst_idx];
    DBG_CHECK(gst_ar);
    if (itf_fifo_full(gst_ar)) {
        return;
    }

    fifo_pop(&axi_demux->ar_fifo, &ar);
    itf_write(gst_ar, &ar);

    axi_demux_rd_ctx_t ctx = { .gst_idx = gst_idx };
    bool alloc_ok = ostk_alloc(&axi_demux->rd_ost, ar.id, &ctx, NULL);
    DBG_CHECK(alloc_ok);
}

static bool axi_demux_find_ready_decerr_b(axi_demux_t *axi_demux,
    u32 *slot)
{
    bool found = false;
    u32 best_slot = 0;
    u64 best_seq = 0;

    for (u32 i = 0; i < axi_demux->wr_rsp_ost.depth; i++) {
        ostk_entry_t *entry = &axi_demux->wr_rsp_ost.entries[i];
        if (!entry->vld || entry->free_pending) {
            continue;
        }

        axi_demux_wr_rsp_ctx_t *ctx =
            (axi_demux_wr_rsp_ctx_t *)entry->ctx;
        if (!ctx->decerr || !ctx->decerr_valid) {
            continue;
        }

        axi_demux_wr_rsp_ctx_t head_ctx;
        u32 head_slot;
        bool head_found = ostk_peek_key(&axi_demux->wr_rsp_ost, entry->key,
            &head_ctx, &head_slot);
        DBG_CHECK(head_found);
        if (head_slot != i) {
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

static void axi_demux_proc_b(axi_demux_t *axi_demux)
{
    if (itf_fifo_full(axi_demux->host_axi4_b_mst)) {
        return;
    }

    u32 decerr_slot;
    if (axi_demux_find_ready_decerr_b(axi_demux, &decerr_slot)) {
        u8 id = (u8)axi_demux->wr_rsp_ost.entries[decerr_slot].key;
        axi4_b_if_t b = { .id = id, .resp = AXI4_B_RESP_DECERR };
        itf_write(axi_demux->host_axi4_b_mst, &b);
        ostk_free_slot(&axi_demux->wr_rsp_ost, decerr_slot);
        return;
    }

    for (u32 i = 0; i < axi_demux->gst_num; i++) {
        u32 gst_idx = (axi_demux->wr_rsp_rr_idx + i) % axi_demux->gst_num;
        itf_t *gst_b = axi_demux->gst_axi4_b_slvs[gst_idx];
        DBG_CHECK(gst_b);
        if (itf_fifo_empty(gst_b)) {
            continue;
        }

        axi4_b_if_t b;
        itf_fifo_get_front(gst_b, &b);

        axi_demux_wr_rsp_ctx_t ctx;
        u32 slot;
        bool found = ostk_peek_key(&axi_demux->wr_rsp_ost, b.id, &ctx, &slot);
        DBG_CHECK(found);
        if (ctx.decerr || ctx.gst_idx != gst_idx) {
            continue;
        }

        itf_fifo_pop_front(gst_b);
        itf_write(axi_demux->host_axi4_b_mst, &b);
        ostk_free_slot(&axi_demux->wr_rsp_ost, slot);
        axi_demux->wr_rsp_rr_idx = (gst_idx + 1u) % axi_demux->gst_num;
        return;
    }
}

static void axi_demux_proc_r(axi_demux_t *axi_demux)
{
    if (itf_fifo_full(axi_demux->host_axi4_r_mst)) {
        return;
    }

    for (u32 i = 0; i < axi_demux->gst_num; i++) {
        u32 gst_idx = (axi_demux->rd_rsp_rr_idx + i) % axi_demux->gst_num;
        itf_t *gst_r = axi_demux->gst_axi4_r_slvs[gst_idx];
        DBG_CHECK(gst_r);
        if (itf_fifo_empty(gst_r)) {
            continue;
        }

        axi4_r_if_t r;
        itf_fifo_get_front(gst_r, &r);

        axi_demux_rd_ctx_t ctx;
        u32 slot;
        bool found = ostk_peek_key(&axi_demux->rd_ost, r.id, &ctx, &slot);
        DBG_CHECK(found);
        if (ctx.gst_idx != gst_idx) {
            continue;
        }

        itf_fifo_pop_front(gst_r);
        itf_write(axi_demux->host_axi4_r_mst, &r);
        if (r.last) {
            ostk_free_slot(&axi_demux->rd_ost, slot);
        }
        axi_demux->rd_rsp_rr_idx = (gst_idx + 1u) % axi_demux->gst_num;
        return;
    }
}

static void axi_demux_proc_w(axi_demux_t *axi_demux)
{
    if (fifo_empty(&axi_demux->w_fifo)) {
        return;
    }

    axi_demux_wr_data_ctx_t ctx;
    u32 data_slot;
    if (!ostq_peek_head(&axi_demux->wr_data_ost, &ctx, &data_slot)) {
        return;
    }

    axi4_w_if_t w;
    fifo_get_front(&axi_demux->w_fifo, &w);

    if (ctx.decerr) {
        fifo_pop(&axi_demux->w_fifo, &w);
        if (w.last) {
            DBG_CHECK(ostk_slot_valid(&axi_demux->wr_rsp_ost, ctx.rsp_slot));
            axi_demux_wr_rsp_ctx_t rsp_ctx;
            ostk_read_slot(&axi_demux->wr_rsp_ost, ctx.rsp_slot, &rsp_ctx);
            DBG_CHECK(rsp_ctx.decerr);
            rsp_ctx.decerr_valid = true;
            ostk_write_slot(&axi_demux->wr_rsp_ost, ctx.rsp_slot, &rsp_ctx);
            ostq_free_head(&axi_demux->wr_data_ost);
        }
        return;
    }

    DBG_CHECK(ctx.gst_idx < axi_demux->gst_num);
    itf_t *gst_w = axi_demux->gst_axi4_w_msts[ctx.gst_idx];
    DBG_CHECK(gst_w);
    if (itf_fifo_full(gst_w)) {
        return;
    }

    fifo_pop(&axi_demux->w_fifo, &w);
    itf_write(gst_w, &w);
    if (w.last) {
        ostq_free_head(&axi_demux->wr_data_ost);
    }
}

static void axi_demux_proc_aw(axi_demux_t *axi_demux)
{
    if (fifo_empty(&axi_demux->aw_fifo)) {
        return;
    }
    if (ostq_full(&axi_demux->wr_data_ost) ||
        ostk_full(&axi_demux->wr_rsp_ost)) {
        return;
    }

    axi4_aw_if_t aw;
    fifo_get_front(&axi_demux->aw_fifo, &aw);

    u32 gst_idx;
    bool hit = axi_demux_decode(axi_demux, aw.addr, &gst_idx);
    if (hit) {
        itf_t *gst_aw = axi_demux->gst_axi4_aw_msts[gst_idx];
        DBG_CHECK(gst_aw);
        if (itf_fifo_full(gst_aw)) {
            return;
        }
    }

    fifo_pop(&axi_demux->aw_fifo, &aw);

    axi_demux_wr_rsp_ctx_t rsp_ctx = {
        .gst_idx = hit ? gst_idx : 0,
        .decerr = !hit,
        .decerr_valid = false
    };
    u32 rsp_slot;
    bool alloc_ok = ostk_alloc(&axi_demux->wr_rsp_ost, aw.id, &rsp_ctx,
        &rsp_slot);
    DBG_CHECK(alloc_ok);

    axi_demux_wr_data_ctx_t data_ctx = {
        .gst_idx = hit ? gst_idx : 0,
        .rsp_slot = rsp_slot,
        .decerr = !hit
    };
    alloc_ok = ostq_alloc(&axi_demux->wr_data_ost, &data_ctx, NULL);
    DBG_CHECK(alloc_ok);

    if (hit) {
        itf_write(axi_demux->gst_axi4_aw_msts[gst_idx], &aw);
    }
}

void axi_demux_clock(axi_demux_t *axi_demux)
{
    mod_clock(&axi_demux->mod);
    ostk_clock(&axi_demux->rd_ost);
    ostq_clock(&axi_demux->wr_data_ost);
    ostk_clock(&axi_demux->wr_rsp_ost);
    axi_demux_capture_ar(axi_demux);
    axi_demux_capture_aw(axi_demux);
    axi_demux_capture_w(axi_demux);
    axi_demux_proc_b(axi_demux);
    axi_demux_proc_r(axi_demux);
    axi_demux_proc_w(axi_demux);
    axi_demux_proc_aw(axi_demux);
    axi_demux_proc_ar(axi_demux);
}

void axi_demux_free(axi_demux_t *axi_demux)
{
    mod_free(&axi_demux->mod);
    fifo_free(&axi_demux->ar_fifo);
    fifo_free(&axi_demux->aw_fifo);
    fifo_free(&axi_demux->w_fifo);
    ostk_free(&axi_demux->rd_ost);
    ostq_free(&axi_demux->wr_data_ost);
    ostk_free(&axi_demux->wr_rsp_ost);
}
