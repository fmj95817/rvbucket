#include "bti_mux.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

void bti_mux_construct(bti_mux_t *bti_mux, const char *parent, const char *name,
    const bti_mux_conf_t *conf)
{
    mod_construct(&bti_mux->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->host_num <= BTI_MUX_HOST_NUM_MAX);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);
    bti_mux->host_num = conf->host_num;
    for (u32 i = 0; i < conf->host_num; i++) {
        fifo_construct(&bti_mux->host_req_fifos[i], sizeof(bti_req_if_t),
            conf->stg_fifo_depth);
    }
    ostk_construct(&bti_mux->ost, sizeof(bti_mux_ost_ctx_t), conf->ost_depth);

    bti_mux->perf_stg_full = dbg_pcm_reg_perf_cnt(bti_mux->mod.hier_name,
        "stg_full");
}

void bti_mux_reset(bti_mux_t *bti_mux)
{
    mod_reset(&bti_mux->mod);
    bti_mux->req_rr_idx = 0;
    for (u32 i = 0; i < bti_mux->host_num; i++) {
        fifo_reset(&bti_mux->host_req_fifos[i]);
    }
    ostk_reset(&bti_mux->ost);
    *bti_mux->perf_stg_full = 0;
}

static void bti_mux_capture_req(bti_mux_t *bti_mux)
{
    for (u32 i = 0; i < bti_mux->host_num; i++) {
        itf_t *host_req = bti_mux->host_bti_req_slvs[i];
        DBG_CHECK(host_req != NULL);
        if (fifo_full(&bti_mux->host_req_fifos[i])) {
            if (!itf_fifo_empty(host_req)) {
                (*bti_mux->perf_stg_full)++;
            }
            continue;
        }

        if (itf_fifo_empty(host_req)) {
            continue;
        }

        bti_req_if_t req;
        itf_read(host_req, &req);
        fifo_push(&bti_mux->host_req_fifos[i], &req);
    }
}

static void bti_mux_proc_req(bti_mux_t *bti_mux)
{
    if (ostk_full(&bti_mux->ost)) {
        return;
    }
    if (itf_fifo_full(bti_mux->gst_bti_req_mst)) {
        return;
    }

    for (u32 i = 0; i < bti_mux->host_num; i++) {
        u32 host_idx = (bti_mux->req_rr_idx + i) % bti_mux->host_num;
        fifo_t *host_req_fifo = &bti_mux->host_req_fifos[host_idx];
        if (fifo_empty(host_req_fifo)) {
            continue;
        }

        bti_req_if_t req;
        fifo_pop(host_req_fifo, &req);

        bti_mux_ost_ctx_t ctx = { .host_idx = host_idx };
        bool alloc_ok = ostk_alloc(&bti_mux->ost, req.trans_id, &ctx, NULL);
        DBG_CHECK(alloc_ok);

        itf_write(bti_mux->gst_bti_req_mst, &req);
        bti_mux->req_rr_idx = (host_idx + 1) % bti_mux->host_num;
        return;
    }
}

static void bti_mux_proc_rsp(bti_mux_t *bti_mux)
{
    if (itf_fifo_empty(bti_mux->gst_bti_rsp_slv)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_fifo_get_front(bti_mux->gst_bti_rsp_slv, &rsp);

    bti_mux_ost_ctx_t ctx;
    u32 slot;
    bool found = ostk_peek_key(&bti_mux->ost, rsp.trans_id, &ctx, &slot);
    DBG_CHECK(found);
    DBG_CHECK(ctx.host_idx < bti_mux->host_num);

    itf_t *host_rsp = bti_mux->host_bti_rsp_msts[ctx.host_idx];
    DBG_CHECK(host_rsp != NULL);
    if (itf_fifo_full(host_rsp)) {
        return;
    }

    itf_read(bti_mux->gst_bti_rsp_slv, &rsp);
    itf_write(host_rsp, &rsp);
    ostk_free_slot(&bti_mux->ost, slot);
}

void bti_mux_clock(bti_mux_t *bti_mux)
{
    mod_clock(&bti_mux->mod);
    ostk_clock(&bti_mux->ost);
    bti_mux_capture_req(bti_mux);
    bti_mux_proc_rsp(bti_mux);
    bti_mux_proc_req(bti_mux);
}

void bti_mux_free(bti_mux_t *bti_mux)
{
    mod_free(&bti_mux->mod);
    for (u32 i = 0; i < bti_mux->host_num; i++) {
        fifo_free(&bti_mux->host_req_fifos[i]);
    }
    ostk_free(&bti_mux->ost);
}
