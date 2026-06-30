#include "axi_mux.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void axi_mux_construct(axi_mux_t *axi_mux, const char *name, u32 host_num)
{
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(host_num <= AXI_MUX_HOST_NUM_MAX);
    axi_mux->host_num = host_num;
}

void axi_mux_reset(axi_mux_t *axi_mux)
{
    axi_mux->rd_state = AXI_MUX_STATE_IDLE;
    axi_mux->rd_active_host_idx = 0;
    axi_mux->rd_rr_idx = 0;

    axi_mux->wr_state = AXI_MUX_STATE_IDLE;
    axi_mux->wr_active_host_idx = 0;
    axi_mux->wr_rr_idx = 0;
    axi_mux->wr_w_done = false;
}

static void axi_mux_proc_b(axi_mux_t *axi_mux)
{
    if (axi_mux->wr_state != AXI_MUX_STATE_WR_RESP) {
        return;
    }
    if (itf_fifo_empty(axi_mux->gst_axi4_b_slv)) {
        return;
    }

    itf_t *host_b = axi_mux->host_axi4_b_msts[axi_mux->wr_active_host_idx];
    DBG_CHECK(host_b != NULL);
    if (itf_fifo_full(host_b)) {
        return;
    }

    axi4_b_if_t b;
    itf_read(axi_mux->gst_axi4_b_slv, &b);
    itf_write(host_b, &b);
    axi_mux->wr_state = AXI_MUX_STATE_IDLE;
}

static void axi_mux_proc_r(axi_mux_t *axi_mux)
{
    if (axi_mux->rd_state != AXI_MUX_STATE_RD) {
        return;
    }
    if (itf_fifo_empty(axi_mux->gst_axi4_r_slv)) {
        return;
    }

    itf_t *host_r = axi_mux->host_axi4_r_msts[axi_mux->rd_active_host_idx];
    DBG_CHECK(host_r != NULL);
    if (itf_fifo_full(host_r)) {
        return;
    }

    axi4_r_if_t r;
    itf_read(axi_mux->gst_axi4_r_slv, &r);
    itf_write(host_r, &r);
    if (r.last) {
        axi_mux->rd_state = AXI_MUX_STATE_IDLE;
    }
}

static void axi_mux_proc_w(axi_mux_t *axi_mux)
{
    if (axi_mux->wr_state != AXI_MUX_STATE_WR_DATA) {
        return;
    }
    if (itf_fifo_full(axi_mux->gst_axi4_w_mst)) {
        return;
    }

    itf_t *host_w = axi_mux->host_axi4_w_slvs[axi_mux->wr_active_host_idx];
    DBG_CHECK(host_w != NULL);
    if (itf_fifo_empty(host_w)) {
        return;
    }

    axi4_w_if_t w;
    itf_read(host_w, &w);
    itf_write(axi_mux->gst_axi4_w_mst, &w);
    if (w.last) {
        axi_mux->wr_w_done = true;
        axi_mux->wr_state = AXI_MUX_STATE_WR_RESP;
    }
}

static void axi_mux_proc_aw(axi_mux_t *axi_mux)
{
    if (axi_mux->wr_state != AXI_MUX_STATE_IDLE) {
        return;
    }
    if (itf_fifo_full(axi_mux->gst_axi4_aw_mst)) {
        return;
    }

    for (u32 i = 0; i < axi_mux->host_num; i++) {
        u32 host_idx = (axi_mux->wr_rr_idx + i) % axi_mux->host_num;

        itf_t *host_aw = axi_mux->host_axi4_aw_slvs[host_idx];
        DBG_CHECK(host_aw != NULL);
        if (itf_fifo_empty(host_aw)) {
            continue;
        }

        axi4_aw_if_t aw;
        itf_read(host_aw, &aw);
        itf_write(axi_mux->gst_axi4_aw_mst, &aw);
        axi_mux->wr_state = AXI_MUX_STATE_WR_DATA;
        axi_mux->wr_active_host_idx = host_idx;
        axi_mux->wr_w_done = false;
        axi_mux->wr_rr_idx = (host_idx + 1) % axi_mux->host_num;
        return;
    }
}

static void axi_mux_proc_ar(axi_mux_t *axi_mux)
{
    if (axi_mux->rd_state != AXI_MUX_STATE_IDLE) {
        return;
    }
    if (itf_fifo_full(axi_mux->gst_axi4_ar_mst)) {
        return;
    }

    for (u32 i = 0; i < axi_mux->host_num; i++) {
        u32 host_idx = (axi_mux->rd_rr_idx + i) % axi_mux->host_num;

        itf_t *host_ar = axi_mux->host_axi4_ar_slvs[host_idx];
        DBG_CHECK(host_ar != NULL);
        if (itf_fifo_empty(host_ar)) {
            continue;
        }

        axi4_ar_if_t ar;
        itf_read(host_ar, &ar);
        itf_write(axi_mux->gst_axi4_ar_mst, &ar);
        axi_mux->rd_state = AXI_MUX_STATE_RD;
        axi_mux->rd_active_host_idx = host_idx;
        axi_mux->rd_rr_idx = (host_idx + 1) % axi_mux->host_num;
        return;
    }
}

void axi_mux_clock(axi_mux_t *axi_mux)
{
    axi_mux_proc_b(axi_mux);
    axi_mux_proc_r(axi_mux);
    axi_mux_proc_w(axi_mux);
    axi_mux_proc_aw(axi_mux);
    axi_mux_proc_ar(axi_mux);
}

void axi_mux_free(axi_mux_t *axi_mux)
{
    (void)axi_mux;
}
