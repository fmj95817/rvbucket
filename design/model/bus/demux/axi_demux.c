#include "axi_demux.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void axi_demux_construct(axi_demux_t *axi_demux, const char *name,
    u32 gst_num, const u32 *gst_bases, const u32 *gst_sizes)
{
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(gst_num <= AXI_DEMUX_GST_NUM_MAX);
    axi_demux->gst_num = gst_num;

    for (u32 i = 0; i < gst_num; i++) {
        axi_demux->gst_bases[i] = gst_bases[i];
        axi_demux->gst_sizes[i] = gst_sizes[i];
    }
}

void axi_demux_reset(axi_demux_t *axi_demux)
{
    axi_demux->rd_state = AXI_DEMUX_STATE_IDLE;
    axi_demux->rd_active_gst_idx = 0;
    axi_demux->wr_state = AXI_DEMUX_STATE_IDLE;
    axi_demux->wr_active_gst_idx = 0;
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

static void axi_demux_proc_r(axi_demux_t *axi_demux)
{
    if (axi_demux->rd_state != AXI_DEMUX_STATE_RD) {
        return;
    }
    if (itf_fifo_full(axi_demux->host_axi4_r_mst)) {
        return;
    }

    itf_t *gst_r = axi_demux->gst_axi4_r_slvs[axi_demux->rd_active_gst_idx];
    DBG_CHECK(gst_r);
    if (itf_fifo_empty(gst_r)) {
        return;
    }

    axi4_r_if_t r;
    itf_read(gst_r, &r);
    itf_write(axi_demux->host_axi4_r_mst, &r);
    if (r.last) {
        axi_demux->rd_state = AXI_DEMUX_STATE_IDLE;
    }
}

static void axi_demux_proc_b(axi_demux_t *axi_demux)
{
    if (axi_demux->wr_state != AXI_DEMUX_STATE_WR_RESP) {
        return;
    }
    if (itf_fifo_full(axi_demux->host_axi4_b_mst)) {
        return;
    }

    itf_t *gst_b = axi_demux->gst_axi4_b_slvs[axi_demux->wr_active_gst_idx];
    DBG_CHECK(gst_b);
    if (itf_fifo_empty(gst_b)) {
        return;
    }

    axi4_b_if_t b;
    itf_read(gst_b, &b);
    itf_write(axi_demux->host_axi4_b_mst, &b);
    axi_demux->wr_state = AXI_DEMUX_STATE_IDLE;
}

static void axi_demux_proc_w(axi_demux_t *axi_demux)
{
    if (axi_demux->wr_state == AXI_DEMUX_STATE_WR_DECERR) {
        if (itf_fifo_empty(axi_demux->host_axi4_w_slv) ||
            itf_fifo_full(axi_demux->host_axi4_b_mst)) {
            return;
        }

        axi4_w_if_t w;
        itf_read(axi_demux->host_axi4_w_slv, &w);
        if (w.last) {
            axi4_b_if_t b = { .id = axi_demux->wr_decerr_id, .resp = AXI4_B_RESP_DECERR };
            itf_write(axi_demux->host_axi4_b_mst, &b);
            axi_demux->wr_state = AXI_DEMUX_STATE_IDLE;
        }
        return;
    }

    if (axi_demux->wr_state != AXI_DEMUX_STATE_WR_DATA) {
        return;
    }

    itf_t *gst_w = axi_demux->gst_axi4_w_msts[axi_demux->wr_active_gst_idx];
    DBG_CHECK(gst_w);
    if (itf_fifo_empty(axi_demux->host_axi4_w_slv) || itf_fifo_full(gst_w)) {
        return;
    }

    axi4_w_if_t w;
    itf_read(axi_demux->host_axi4_w_slv, &w);
    itf_write(gst_w, &w);
    if (w.last) {
        axi_demux->wr_state = AXI_DEMUX_STATE_WR_RESP;
    }
}

static void axi_demux_proc_aw(axi_demux_t *axi_demux)
{
    if (axi_demux->wr_state != AXI_DEMUX_STATE_IDLE) {
        return;
    }
    if (itf_fifo_empty(axi_demux->host_axi4_aw_slv)) {
        return;
    }

    axi4_aw_if_t aw;
    itf_fifo_get_front(axi_demux->host_axi4_aw_slv, &aw);
    u32 gst_idx;
    if (!axi_demux_decode(axi_demux, aw.addr, &gst_idx)) {
        itf_fifo_pop_front(axi_demux->host_axi4_aw_slv);
        axi_demux->wr_decerr_id = aw.id;
        axi_demux->wr_state = AXI_DEMUX_STATE_WR_DECERR;
        return;
    }

    itf_t *gst_aw = axi_demux->gst_axi4_aw_msts[gst_idx];
    DBG_CHECK(gst_aw);
    if (itf_fifo_full(gst_aw)) {
        return;
    }

    itf_fifo_pop_front(axi_demux->host_axi4_aw_slv);
    itf_write(gst_aw, &aw);
    axi_demux->wr_active_gst_idx = gst_idx;
    axi_demux->wr_state = AXI_DEMUX_STATE_WR_DATA;
}

static void axi_demux_proc_ar(axi_demux_t *axi_demux)
{
    if (axi_demux->rd_state != AXI_DEMUX_STATE_IDLE) {
        return;
    }
    if (itf_fifo_empty(axi_demux->host_axi4_ar_slv)) {
        return;
    }

    axi4_ar_if_t ar;
    itf_fifo_get_front(axi_demux->host_axi4_ar_slv, &ar);
    u32 gst_idx;
    if (!axi_demux_decode(axi_demux, ar.addr, &gst_idx)) {
        if (itf_fifo_full(axi_demux->host_axi4_r_mst)) {
            return;
        }
        itf_fifo_pop_front(axi_demux->host_axi4_ar_slv);
        axi4_r_if_t r = {
            .id = ar.id,
            .data = 0,
            .resp = AXI4_R_RESP_DECERR,
            .last = true
        };
        itf_write(axi_demux->host_axi4_r_mst, &r);
        return;
    }

    itf_t *gst_ar = axi_demux->gst_axi4_ar_msts[gst_idx];
    DBG_CHECK(gst_ar);
    if (itf_fifo_full(gst_ar)) {
        return;
    }

    itf_fifo_pop_front(axi_demux->host_axi4_ar_slv);
    itf_write(gst_ar, &ar);
    axi_demux->rd_active_gst_idx = gst_idx;
    axi_demux->rd_state = AXI_DEMUX_STATE_RD;
}

void axi_demux_clock(axi_demux_t *axi_demux)
{
    axi_demux_proc_b(axi_demux);
    axi_demux_proc_r(axi_demux);
    axi_demux_proc_w(axi_demux);
    axi_demux_proc_aw(axi_demux);
    axi_demux_proc_ar(axi_demux);
}

void axi_demux_free(axi_demux_t *axi_demux)
{
    (void)axi_demux;
}
