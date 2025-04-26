#include "demux.h"
#include "dbg/chk.h"

void bti_demux_construct(bti_demux_t *bti_demux, u32 gst_num, const u32 *gst_base_addrs, const u32 *gst_sizes)
{
    DBG_CHECK(gst_num <= BTI_MUX_GST_NUM_MAX);
    bti_demux->gst_num = gst_num;

    for (u32 i = 0; i < gst_num; i++) {
        bti_demux->gst_base_addrs[i] = gst_base_addrs[i];
        bti_demux->gst_sizes[i] = gst_sizes[i];
    }
}

void bti_demux_reset(bti_demux_t *bti_demux) {}

static void bti_demux_proc_req(bti_demux_t *bti_demux)
{
    if (itf_fifo_empty(bti_demux->host_bti_req_slv)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_fifo_get_front(bti_demux->host_bti_req_slv, &bti_req);

    u32 gst_idx;
    for (gst_idx = 0; gst_idx < bti_demux->gst_num; gst_idx++) {
        u32 base_addr = bti_demux->gst_base_addrs[gst_idx];
        u32 size = bti_demux->gst_sizes[gst_idx];
        if (ADDR_IN(bti_req.addr, base_addr, size)) {
            break;
        }
    }
    DBG_CHECK(gst_idx < bti_demux->gst_num);

    itf_t *gst_bti_req_mst = bti_demux->gst_bti_req_msts[gst_idx];
    if (itf_fifo_full(gst_bti_req_mst)) {
        return;
    }

    itf_fifo_pop_front(bti_demux->host_bti_req_slv);
    itf_write(gst_bti_req_mst, &bti_req);
}

static void bti_demux_proc_rsp(bti_demux_t *bti_demux)
{
    for (u32 i = 0; i < bti_demux->gst_num; i++) {
        if (itf_fifo_full(bti_demux->host_bti_rsp_mst)) {
            continue;
        }

        if (itf_fifo_empty(bti_demux->gst_bti_rsp_slvs[i])) {
            continue;
        }

        bti_rsp_if_t bti_rsp;
        itf_read(bti_demux->gst_bti_rsp_slvs[i], &bti_rsp);
        itf_write(bti_demux->host_bti_rsp_mst, &bti_rsp);
    }
}

void bti_demux_clock(bti_demux_t *bti_demux)
{
    bti_demux_proc_req(bti_demux);
    bti_demux_proc_rsp(bti_demux);
}

void bti_demux_free(bti_demux_t *bti_demux) {}
