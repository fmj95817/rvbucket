#include "bti2axi.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void bti2axi_construct(bti2axi_t *br, const char *parent, const char *name)
{
    mod_construct(&br->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(br->bti_req_slv);
    DBG_CHECK(br->bti_rsp_mst);
    DBG_CHECK(br->axi4_aw_mst);
    DBG_CHECK(br->axi4_w_mst);
    DBG_CHECK(br->axi4_b_slv);
    DBG_CHECK(br->axi4_ar_mst);
    DBG_CHECK(br->axi4_r_slv);
}

void bti2axi_reset(bti2axi_t *br)
{
    mod_reset(&br->mod);
    br->state = BTI2AXI4_STATE_IDLE;
    br->bti_trans_id = 0;
}

static void bti2axi_proc_req(bti2axi_t *br)
{
    if (br->state != BTI2AXI4_STATE_IDLE) {
        return;
    }

    if (itf_fifo_empty(br->bti_req_slv)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_fifo_get_front(br->bti_req_slv, &bti_req);

    if (bti_req.cmd == BTI_REQ_CMD_READ) {
        if (itf_fifo_full(br->axi4_ar_mst)) {
            return;
        }

        itf_fifo_pop_front(br->bti_req_slv);

        axi4_ar_if_t ar = {
            .id = 0,
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
        itf_write(br->axi4_ar_mst, &ar);
        br->bti_trans_id = bti_req.trans_id;
        br->state = BTI2AXI4_STATE_RD_PENDING;
    } else if (bti_req.cmd == BTI_REQ_CMD_WRITE) {
        if (itf_fifo_full(br->axi4_aw_mst) || itf_fifo_full(br->axi4_w_mst)) {
            return;
        }

        itf_fifo_pop_front(br->bti_req_slv);

        axi4_aw_if_t aw = {
            .id = 0,
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
        itf_write(br->axi4_aw_mst, &aw);
        itf_write(br->axi4_w_mst, &w);
        br->bti_trans_id = bti_req.trans_id;
        br->state = BTI2AXI4_STATE_WR_PENDING;
    } else {
        if (itf_fifo_full(br->bti_rsp_mst)) {
            return;
        }
        itf_fifo_pop_front(br->bti_req_slv);
        bti_rsp_if_t rsp = {
            .trans_id = bti_req.trans_id,
            .data = 0,
            .ok = false
        };
        itf_write(br->bti_rsp_mst, &rsp);
    }
}

static void bti2axi_proc_rd_rsp(bti2axi_t *br)
{
    if (br->state != BTI2AXI4_STATE_RD_PENDING) {
        return;
    }

    if (itf_fifo_empty(br->axi4_r_slv)) {
        return;
    }

    if (itf_fifo_full(br->bti_rsp_mst)) {
        return;
    }

    axi4_r_if_t r;
    itf_read(br->axi4_r_slv, &r);

    bti_rsp_if_t bti_rsp = {
        .data = r.data,
        .ok = (r.resp == 0),
        .trans_id = br->bti_trans_id
    };
    itf_write(br->bti_rsp_mst, &bti_rsp);
    br->state = BTI2AXI4_STATE_IDLE;
}

static void bti2axi_proc_wr_rsp(bti2axi_t *br)
{
    if (br->state != BTI2AXI4_STATE_WR_PENDING) {
        return;
    }

    if (itf_fifo_empty(br->axi4_b_slv)) {
        return;
    }

    if (itf_fifo_full(br->bti_rsp_mst)) {
        return;
    }

    axi4_b_if_t b;
    itf_read(br->axi4_b_slv, &b);

    bti_rsp_if_t bti_rsp = {
        .data = 0,
        .ok = (b.resp == 0),
        .trans_id = br->bti_trans_id
    };
    itf_write(br->bti_rsp_mst, &bti_rsp);
    br->state = BTI2AXI4_STATE_IDLE;
}

void bti2axi_clock(bti2axi_t *br)
{
    mod_clock(&br->mod);
    bti2axi_proc_req(br);
    bti2axi_proc_rd_rsp(br);
    bti2axi_proc_wr_rsp(br);
}

void bti2axi_free(bti2axi_t *br)
{
    mod_free(&br->mod);
}
