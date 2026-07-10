#include "bti2axi.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"

void bti2axi_construct(bti2axi_t *br, const char *parent, const char *name,
    const bti2axi_conf_t *conf)
{
    mod_construct(&br->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);

    DBG_CHECK(br->bti_req_slv);
    DBG_CHECK(br->bti_rsp_mst);
    DBG_CHECK(br->axi4_aw_mst);
    DBG_CHECK(br->axi4_w_mst);
    DBG_CHECK(br->axi4_b_slv);
    DBG_CHECK(br->axi4_ar_mst);
    DBG_CHECK(br->axi4_r_slv);

    fifo_construct(&br->bti_req_fifo, sizeof(bti_req_if_t),
        conf->stg_fifo_depth);
    ostk_construct(&br->rd_ost, sizeof(bti2axi_ost_ctx_t),
        conf->ost_depth);
    ostk_construct(&br->wr_ost, sizeof(bti2axi_ost_ctx_t),
        conf->ost_depth);

    br->perf_stg_full = dbg_pcm_reg_perf_cnt(br->mod.hier_name, "stg_full");
}

void bti2axi_reset(bti2axi_t *br)
{
    mod_reset(&br->mod);
    fifo_reset(&br->bti_req_fifo);
    ostk_reset(&br->rd_ost);
    ostk_reset(&br->wr_ost);
    *br->perf_stg_full = 0;
}

static void bti2axi_capture_req(bti2axi_t *br)
{
    if (fifo_full(&br->bti_req_fifo)) {
        if (!itf_fifo_empty(br->bti_req_slv)) {
            (*br->perf_stg_full)++;
        }
        return;
    }

    if (itf_fifo_empty(br->bti_req_slv)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(br->bti_req_slv, &bti_req);
    fifo_push(&br->bti_req_fifo, &bti_req);
}

static void bti2axi_proc_req(bti2axi_t *br)
{
    if (fifo_empty(&br->bti_req_fifo)) {
        return;
    }

    bti_req_if_t bti_req;
    fifo_get_front(&br->bti_req_fifo, &bti_req);

    if (bti_req.cmd == BTI_REQ_CMD_READ) {
        if (ostk_full(&br->rd_ost)) {
            return;
        }
        if (itf_fifo_full(br->axi4_ar_mst)) {
            return;
        }

        fifo_pop(&br->bti_req_fifo, &bti_req);

        axi4_ar_if_t ar = {
            .id = BTI2AXI_AXI_ID,
            .addr = bti_req.addr,
            .len = 0,
            .size = (axi4_ar_size_t)bti_req.size,
            .burst = AXI4_AR_BURST_FIXED,
            .lock = false,
            .cache = 0,
            .prot = 0,
            .qos = 0,
            .user = 0
        };
        bti2axi_ost_ctx_t ctx = { .bti_trans_id = bti_req.trans_id };
        bool alloc_ok = ostk_alloc(&br->rd_ost, ar.id, &ctx, NULL);
        DBG_CHECK(alloc_ok);
        itf_write(br->axi4_ar_mst, &ar);
    } else {
        if (ostk_full(&br->wr_ost)) {
            return;
        }
        if (itf_fifo_full(br->axi4_aw_mst) || itf_fifo_full(br->axi4_w_mst)) {
            return;
        }

        fifo_pop(&br->bti_req_fifo, &bti_req);

        axi4_aw_if_t aw = {
            .id = BTI2AXI_AXI_ID,
            .addr = bti_req.addr,
            .len = 0,
            .size = (axi4_aw_size_t)bti_req.size,
            .burst = AXI4_AW_BURST_FIXED,
            .lock = false,
            .cache = 0,
            .prot = 0,
            .qos = 0,
            .user = 0
        };
        axi4_w_if_t w = {
            .data = bti_req.data,
            .strb = (u8)(bti_req.strobe & 0xf),
            .last = true
        };
        bti2axi_ost_ctx_t ctx = { .bti_trans_id = bti_req.trans_id };
        bool alloc_ok = ostk_alloc(&br->wr_ost, aw.id, &ctx, NULL);
        DBG_CHECK(alloc_ok);
        itf_write(br->axi4_aw_mst, &aw);
        itf_write(br->axi4_w_mst, &w);
    }
}

static void bti2axi_proc_rd_rsp(bti2axi_t *br)
{
    if (itf_fifo_empty(br->axi4_r_slv)) {
        return;
    }

    if (itf_fifo_full(br->bti_rsp_mst)) {
        return;
    }

    axi4_r_if_t r;
    itf_fifo_get_front(br->axi4_r_slv, &r);
    DBG_CHECK(r.last);

    bti2axi_ost_ctx_t ctx;
    u32 slot;
    bool found = ostk_peek_key(&br->rd_ost, r.id, &ctx, &slot);
    DBG_CHECK(found);

    itf_read(br->axi4_r_slv, &r);

    bti_rsp_if_t bti_rsp = {
        .data = r.data,
        .ok = (r.resp == 0),
        .trans_id = ctx.bti_trans_id
    };
    itf_write(br->bti_rsp_mst, &bti_rsp);
    ostk_free_slot(&br->rd_ost, slot);
}

static void bti2axi_proc_wr_rsp(bti2axi_t *br)
{
    if (itf_fifo_empty(br->axi4_b_slv)) {
        return;
    }

    if (itf_fifo_full(br->bti_rsp_mst)) {
        return;
    }

    axi4_b_if_t b;
    itf_fifo_get_front(br->axi4_b_slv, &b);

    bti2axi_ost_ctx_t ctx;
    u32 slot;
    bool found = ostk_peek_key(&br->wr_ost, b.id, &ctx, &slot);
    DBG_CHECK(found);

    itf_read(br->axi4_b_slv, &b);

    bti_rsp_if_t bti_rsp = {
        .data = 0,
        .ok = (b.resp == 0),
        .trans_id = ctx.bti_trans_id
    };
    itf_write(br->bti_rsp_mst, &bti_rsp);
    ostk_free_slot(&br->wr_ost, slot);
}

void bti2axi_clock(bti2axi_t *br)
{
    mod_clock(&br->mod);
    ostk_clock(&br->rd_ost);
    ostk_clock(&br->wr_ost);
    bti2axi_capture_req(br);
    bti2axi_proc_rd_rsp(br);
    bti2axi_proc_wr_rsp(br);
    bti2axi_proc_req(br);
}

void bti2axi_free(bti2axi_t *br)
{
    mod_free(&br->mod);
    fifo_free(&br->bti_req_fifo);
    ostk_free(&br->rd_ost);
    ostk_free(&br->wr_ost);
}
