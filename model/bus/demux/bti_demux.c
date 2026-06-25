#include "bti_demux.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void bti_demux_construct(bti_demux_t *bti_demux, const char *name,
    u32 gst_num, const u32 *gst_bases, const u32 *gst_sizes)
{
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(gst_num <= BTI_DEMUX_GST_NUM_MAX);
    bti_demux->gst_num = gst_num;

    for (u32 i = 0; i < gst_num; i++) {
        bti_demux->gst_bases[i] = gst_bases[i];
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

    itf_fifo_pop_front(bti_demux->host_bti_req_slv);
    itf_write(gst_bti_req_mst, &bti_req);
}

static void bti_demux_proc_rsp(bti_demux_t *bti_demux)
{
    for (u32 i = 0; i < bti_demux->gst_num; i++) {
        itf_t *gst_bti_rsp_slv = bti_demux->gst_bti_rsp_slvs[i];
        if (gst_bti_rsp_slv == NULL) {
            continue;
        }

        if (itf_fifo_full(bti_demux->host_bti_rsp_mst)) {
            continue;
        }

        if (itf_fifo_empty(gst_bti_rsp_slv)) {
            continue;
        }

        bti_rsp_if_t bti_rsp;
        itf_read(gst_bti_rsp_slv, &bti_rsp);
        itf_write(bti_demux->host_bti_rsp_mst, &bti_rsp);
    }
}

void bti_demux_clock(bti_demux_t *bti_demux)
{
    bti_demux_proc_req(bti_demux);
    bti_demux_proc_rsp(bti_demux);
}

void bti_demux_free(bti_demux_t *bti_demux) {}
