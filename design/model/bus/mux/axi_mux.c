#include "axi_mux.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

void axi_mux_construct(axi_mux_t *axi_mux, const char *parent, const char *name,
    const axi_mux_conf_t *conf)
{
    mod_construct(&axi_mux->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->host_num <= AXI_MUX_HOST_NUM_MAX);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);
    axi_mux->host_num = conf->host_num;
    for (u32 i = 0; i < conf->host_num; i++) {
        fifo_construct(&axi_mux->host_ar_fifos[i], sizeof(axi4_ar_if_t),
            conf->stg_fifo_depth);
        fifo_construct(&axi_mux->host_aw_fifos[i], sizeof(axi4_aw_if_t),
            conf->stg_fifo_depth);
        fifo_construct(&axi_mux->host_w_fifos[i], sizeof(axi4_w_if_t),
            conf->stg_fifo_depth);
    }
    ostk_construct(&axi_mux->rd_ost, sizeof(axi_mux_ost_ctx_t),
        conf->ost_depth);
    ostq_construct(&axi_mux->wr_data_ost, sizeof(axi_mux_ost_ctx_t),
        conf->ost_depth);
    ostk_construct(&axi_mux->wr_rsp_ost, sizeof(axi_mux_ost_ctx_t),
        conf->ost_depth);

    axi_mux->perf_stg_ar_full = dbg_pcm_reg_perf_cnt(axi_mux->mod.hier_name,
        "stg_ar_full");
    axi_mux->perf_stg_aw_full = dbg_pcm_reg_perf_cnt(axi_mux->mod.hier_name,
        "stg_aw_full");
    axi_mux->perf_stg_w_full = dbg_pcm_reg_perf_cnt(axi_mux->mod.hier_name,
        "stg_w_full");
}

void axi_mux_reset(axi_mux_t *axi_mux)
{
    mod_reset(&axi_mux->mod);
    axi_mux->rd_rr_idx = 0;
    axi_mux->wr_rr_idx = 0;
    for (u32 i = 0; i < axi_mux->host_num; i++) {
        fifo_reset(&axi_mux->host_ar_fifos[i]);
        fifo_reset(&axi_mux->host_aw_fifos[i]);
        fifo_reset(&axi_mux->host_w_fifos[i]);
    }
    ostk_reset(&axi_mux->rd_ost);
    ostq_reset(&axi_mux->wr_data_ost);
    ostk_reset(&axi_mux->wr_rsp_ost);
    *axi_mux->perf_stg_ar_full = 0;
    *axi_mux->perf_stg_aw_full = 0;
    *axi_mux->perf_stg_w_full = 0;
}

static void axi_mux_capture_ar(axi_mux_t *axi_mux)
{
    for (u32 i = 0; i < axi_mux->host_num; i++) {
        itf_t *host_ar = axi_mux->host_axi4_ar_slvs[i];
        DBG_CHECK(host_ar != NULL);
        if (fifo_full(&axi_mux->host_ar_fifos[i])) {
            if (!itf_fifo_empty(host_ar)) {
                (*axi_mux->perf_stg_ar_full)++;
            }
            continue;
        }

        if (itf_fifo_empty(host_ar)) {
            continue;
        }

        axi4_ar_if_t ar;
        itf_read(host_ar, &ar);
        fifo_push(&axi_mux->host_ar_fifos[i], &ar);
    }
}

static void axi_mux_capture_aw(axi_mux_t *axi_mux)
{
    for (u32 i = 0; i < axi_mux->host_num; i++) {
        itf_t *host_aw = axi_mux->host_axi4_aw_slvs[i];
        DBG_CHECK(host_aw != NULL);
        if (fifo_full(&axi_mux->host_aw_fifos[i])) {
            if (!itf_fifo_empty(host_aw)) {
                (*axi_mux->perf_stg_aw_full)++;
            }
            continue;
        }

        if (itf_fifo_empty(host_aw)) {
            continue;
        }

        axi4_aw_if_t aw;
        itf_read(host_aw, &aw);
        fifo_push(&axi_mux->host_aw_fifos[i], &aw);
    }
}

static void axi_mux_capture_w(axi_mux_t *axi_mux)
{
    for (u32 i = 0; i < axi_mux->host_num; i++) {
        itf_t *host_w = axi_mux->host_axi4_w_slvs[i];
        DBG_CHECK(host_w != NULL);
        if (fifo_full(&axi_mux->host_w_fifos[i])) {
            if (!itf_fifo_empty(host_w)) {
                (*axi_mux->perf_stg_w_full)++;
            }
            continue;
        }

        if (itf_fifo_empty(host_w)) {
            continue;
        }

        axi4_w_if_t w;
        itf_read(host_w, &w);
        fifo_push(&axi_mux->host_w_fifos[i], &w);
    }
}

static void axi_mux_proc_b(axi_mux_t *axi_mux)
{
    if (itf_fifo_empty(axi_mux->gst_axi4_b_slv)) {
        return;
    }

    axi4_b_if_t b;
    itf_fifo_get_front(axi_mux->gst_axi4_b_slv, &b);

    axi_mux_ost_ctx_t ctx;
    u32 slot;
    bool found = ostk_peek_key(&axi_mux->wr_rsp_ost, b.id, &ctx, &slot);
    DBG_CHECK(found);
    DBG_CHECK(ctx.host_idx < axi_mux->host_num);

    itf_t *host_b = axi_mux->host_axi4_b_msts[ctx.host_idx];
    DBG_CHECK(host_b != NULL);
    if (itf_fifo_full(host_b)) {
        return;
    }

    itf_fifo_pop_front(axi_mux->gst_axi4_b_slv);
    itf_write(host_b, &b);
    ostk_free_slot(&axi_mux->wr_rsp_ost, slot);
}

static void axi_mux_proc_r(axi_mux_t *axi_mux)
{
    if (itf_fifo_empty(axi_mux->gst_axi4_r_slv)) {
        return;
    }

    axi4_r_if_t r;
    itf_fifo_get_front(axi_mux->gst_axi4_r_slv, &r);

    axi_mux_ost_ctx_t ctx;
    u32 slot;
    bool found = ostk_peek_key(&axi_mux->rd_ost, r.id, &ctx, &slot);
    DBG_CHECK(found);
    DBG_CHECK(ctx.host_idx < axi_mux->host_num);

    itf_t *host_r = axi_mux->host_axi4_r_msts[ctx.host_idx];
    DBG_CHECK(host_r != NULL);
    if (itf_fifo_full(host_r)) {
        return;
    }

    itf_fifo_pop_front(axi_mux->gst_axi4_r_slv);
    itf_write(host_r, &r);
    if (r.last) {
        ostk_free_slot(&axi_mux->rd_ost, slot);
    }
}

static void axi_mux_proc_w(axi_mux_t *axi_mux)
{
    if (itf_fifo_full(axi_mux->gst_axi4_w_mst)) {
        return;
    }

    axi_mux_ost_ctx_t ctx;
    if (!ostq_peek_head(&axi_mux->wr_data_ost, &ctx, NULL)) {
        return;
    }
    DBG_CHECK(ctx.host_idx < axi_mux->host_num);

    fifo_t *host_w_fifo = &axi_mux->host_w_fifos[ctx.host_idx];
    if (fifo_empty(host_w_fifo)) {
        return;
    }

    axi4_w_if_t w;
    fifo_pop(host_w_fifo, &w);
    itf_write(axi_mux->gst_axi4_w_mst, &w);
    if (w.last) {
        ostq_free_head(&axi_mux->wr_data_ost);
    }
}

static void axi_mux_proc_aw(axi_mux_t *axi_mux)
{
    if (ostq_full(&axi_mux->wr_data_ost) ||
        ostk_full(&axi_mux->wr_rsp_ost)) {
        return;
    }
    if (itf_fifo_full(axi_mux->gst_axi4_aw_mst)) {
        return;
    }

    for (u32 i = 0; i < axi_mux->host_num; i++) {
        u32 host_idx = (axi_mux->wr_rr_idx + i) % axi_mux->host_num;

        fifo_t *host_aw_fifo = &axi_mux->host_aw_fifos[host_idx];
        if (fifo_empty(host_aw_fifo)) {
            continue;
        }

        axi4_aw_if_t aw;
        fifo_get_front(host_aw_fifo, &aw);

        axi_mux_ost_ctx_t ctx = { .host_idx = host_idx };
        bool alloc_ok = ostq_alloc(&axi_mux->wr_data_ost, &ctx, NULL);
        DBG_CHECK(alloc_ok);
        alloc_ok = ostk_alloc(&axi_mux->wr_rsp_ost, aw.id, &ctx, NULL);
        DBG_CHECK(alloc_ok);

        fifo_pop(host_aw_fifo, &aw);
        itf_write(axi_mux->gst_axi4_aw_mst, &aw);
        axi_mux->wr_rr_idx = (host_idx + 1) % axi_mux->host_num;
        return;
    }
}

static void axi_mux_proc_ar(axi_mux_t *axi_mux)
{
    if (ostk_full(&axi_mux->rd_ost)) {
        return;
    }
    if (itf_fifo_full(axi_mux->gst_axi4_ar_mst)) {
        return;
    }

    for (u32 i = 0; i < axi_mux->host_num; i++) {
        u32 host_idx = (axi_mux->rd_rr_idx + i) % axi_mux->host_num;

        fifo_t *host_ar_fifo = &axi_mux->host_ar_fifos[host_idx];
        if (fifo_empty(host_ar_fifo)) {
            continue;
        }

        axi4_ar_if_t ar;
        fifo_get_front(host_ar_fifo, &ar);

        axi_mux_ost_ctx_t ctx = { .host_idx = host_idx };
        bool alloc_ok = ostk_alloc(&axi_mux->rd_ost, ar.id, &ctx, NULL);
        DBG_CHECK(alloc_ok);

        fifo_pop(host_ar_fifo, &ar);
        itf_write(axi_mux->gst_axi4_ar_mst, &ar);
        axi_mux->rd_rr_idx = (host_idx + 1) % axi_mux->host_num;
        return;
    }
}

void axi_mux_clock(axi_mux_t *axi_mux)
{
    mod_clock(&axi_mux->mod);
    ostk_clock(&axi_mux->rd_ost);
    ostq_clock(&axi_mux->wr_data_ost);
    ostk_clock(&axi_mux->wr_rsp_ost);
    axi_mux_capture_ar(axi_mux);
    axi_mux_capture_aw(axi_mux);
    axi_mux_capture_w(axi_mux);
    axi_mux_proc_b(axi_mux);
    axi_mux_proc_r(axi_mux);
    axi_mux_proc_w(axi_mux);
    axi_mux_proc_aw(axi_mux);
    axi_mux_proc_ar(axi_mux);
}

void axi_mux_free(axi_mux_t *axi_mux)
{
    mod_free(&axi_mux->mod);
    for (u32 i = 0; i < axi_mux->host_num; i++) {
        fifo_free(&axi_mux->host_ar_fifos[i]);
        fifo_free(&axi_mux->host_aw_fifos[i]);
        fifo_free(&axi_mux->host_w_fifos[i]);
    }
    ostk_free(&axi_mux->rd_ost);
    ostq_free(&axi_mux->wr_data_ost);
    ostk_free(&axi_mux->wr_rsp_ost);
}
