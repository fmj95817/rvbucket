#include "bti_demux.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

static u32 bti_demux_ost_key(u32 gst_idx, u16 trans_id)
{
    return (gst_idx << 16u) | trans_id;
}

void bti_demux_construct(bti_demux_t *bti_demux, const char *parent, const char *name,
    const bti_demux_conf_t *conf)
{
    mod_construct(&bti_demux->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(conf);
    DBG_CHECK(conf->gst_num <= BTI_DEMUX_GST_NUM_MAX);
    DBG_CHECK(conf->gst_bases);
    DBG_CHECK(conf->gst_sizes);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);
    bti_demux->gst_num = conf->gst_num;

    for (u32 i = 0; i < conf->gst_num; i++) {
        bti_demux->gst_bases[i] = conf->gst_bases[i];
        bti_demux->gst_sizes[i] = conf->gst_sizes[i];
    }
    fifo_construct(&bti_demux->req_fifo, sizeof(bti_req_if_t),
        conf->stg_fifo_depth);
    ostk_construct(&bti_demux->ost, sizeof(bti_demux_ost_ctx_t),
        conf->ost_depth);

    bti_demux->perf_stg_full = dbg_pcm_reg_perf_cnt(bti_demux->mod.hier_name,
        "stg_full");
}

void bti_demux_reset(bti_demux_t *bti_demux)
{
    mod_reset(&bti_demux->mod);
    fifo_reset(&bti_demux->req_fifo);
    ostk_reset(&bti_demux->ost);
    *bti_demux->perf_stg_full = 0;
}

static void bti_demux_capture_req(bti_demux_t *bti_demux)
{
    if (fifo_full(&bti_demux->req_fifo)) {
        if (!itf_fifo_empty(bti_demux->host_bti_req_slv)) {
            (*bti_demux->perf_stg_full)++;
        }
        return;
    }

    if (itf_fifo_empty(bti_demux->host_bti_req_slv)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(bti_demux->host_bti_req_slv, &bti_req);
    fifo_push(&bti_demux->req_fifo, &bti_req);
}

static void bti_demux_proc_req(bti_demux_t *bti_demux)
{
    if (ostk_full(&bti_demux->ost)) {
        return;
    }

    if (fifo_empty(&bti_demux->req_fifo)) {
        return;
    }

    bti_req_if_t bti_req;
    fifo_get_front(&bti_demux->req_fifo, &bti_req);

    u32 gst_idx;
    for (gst_idx = 0; gst_idx < bti_demux->gst_num; gst_idx++) {
        u32 base_addr = bti_demux->gst_bases[gst_idx];
        u32 size = bti_demux->gst_sizes[gst_idx];
        if (ADDR_IN(bti_req.addr, base_addr, size)) {
            break;
        }
    }
    DBG_CHECK(gst_idx < bti_demux->gst_num);

    itf_t *gst_bti_req_mst = bti_demux->gst_bti_req_msts[gst_idx];
    DBG_CHECK(gst_bti_req_mst);

    if (itf_fifo_full(gst_bti_req_mst)) {
        return;
    }

    fifo_pop(&bti_demux->req_fifo, &bti_req);
    bti_demux_ost_ctx_t ctx = { .gst_idx = gst_idx };
    bool alloc_ok = ostk_alloc(&bti_demux->ost,
        bti_demux_ost_key(gst_idx, bti_req.trans_id), &ctx, NULL);
    DBG_CHECK(alloc_ok);
    itf_write(gst_bti_req_mst, &bti_req);
}

static void bti_demux_proc_rsp(bti_demux_t *bti_demux)
{
    if (itf_fifo_full(bti_demux->host_bti_rsp_mst)) {
        return;
    }

    for (u32 i = 0; i < bti_demux->gst_num; i++) {
        itf_t *gst_bti_rsp_slv = bti_demux->gst_bti_rsp_slvs[i];
        if (gst_bti_rsp_slv == NULL) {
            continue;
        }

        if (itf_fifo_empty(gst_bti_rsp_slv)) {
            continue;
        }

        bti_rsp_if_t bti_rsp;
        itf_fifo_get_front(gst_bti_rsp_slv, &bti_rsp);

        bti_demux_ost_ctx_t ctx;
        u32 slot;
        bool found = ostk_peek_key(&bti_demux->ost,
            bti_demux_ost_key(i, bti_rsp.trans_id), &ctx, &slot);
        DBG_CHECK(found);
        DBG_CHECK(ctx.gst_idx == i);

        itf_read(gst_bti_rsp_slv, &bti_rsp);
        itf_write(bti_demux->host_bti_rsp_mst, &bti_rsp);
        ostk_free_slot(&bti_demux->ost, slot);

        if (itf_fifo_full(bti_demux->host_bti_rsp_mst)) {
            return;
        }
    }
}

void bti_demux_clock(bti_demux_t *bti_demux)
{
    mod_clock(&bti_demux->mod);
    ostk_clock(&bti_demux->ost);
    bti_demux_capture_req(bti_demux);
    bti_demux_proc_req(bti_demux);
    bti_demux_proc_rsp(bti_demux);
}

void bti_demux_free(bti_demux_t *bti_demux)
{
    mod_free(&bti_demux->mod);
    fifo_free(&bti_demux->req_fifo);
    ostk_free(&bti_demux->ost);
}
