#include "apb_demux.h"
#include "base/def.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void apb_demux_construct(apb_demux_t *apb_demux, const char *name,
    u32 gst_num, const u32 *gst_bases, const u32 *gst_sizes)
{
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(gst_num <= APB_DEMUX_GST_NUM_MAX);
    apb_demux->gst_num = gst_num;

    for (u32 i = 0; i < gst_num; i++) {
        apb_demux->gst_bases[i] = gst_bases[i];
        apb_demux->gst_sizes[i] = gst_sizes[i];
    }
}

void apb_demux_reset(apb_demux_t *apb_demux) {}

static void apb_demux_proc_req(apb_demux_t *apb_demux)
{
    if (itf_fifo_empty(apb_demux->host_apb_req_slv)) {
        return;
    }

    apb_req_if_t apb_req;
    itf_fifo_get_front(apb_demux->host_apb_req_slv, &apb_req);

    u32 gst_idx;
    for (gst_idx = 0; gst_idx < apb_demux->gst_num; gst_idx++) {
        u32 base_addr = apb_demux->gst_bases[gst_idx];
        u32 size = apb_demux->gst_sizes[gst_idx];
        if (ADDR_IN(apb_req.paddr, base_addr, size)) {
            break;
        }
    }
    DBG_CHECK(gst_idx < apb_demux->gst_num);

    itf_t *gst_apb_req_mst = apb_demux->gst_apb_req_msts[gst_idx];
    DBG_CHECK(gst_apb_req_mst);

    if (itf_fifo_full(gst_apb_req_mst)) {
        return;
    }

    itf_fifo_pop_front(apb_demux->host_apb_req_slv);
    itf_write(gst_apb_req_mst, &apb_req);
}

static void apb_demux_proc_rsp(apb_demux_t *apb_demux)
{
    for (u32 i = 0; i < apb_demux->gst_num; i++) {
        itf_t *gst_apb_rsp_slv = apb_demux->gst_apb_rsp_slvs[i];
        if (gst_apb_rsp_slv == NULL) {
            continue;
        }

        if (itf_fifo_full(apb_demux->host_apb_rsp_mst)) {
            continue;
        }

        if (itf_fifo_empty(gst_apb_rsp_slv)) {
            continue;
        }

        apb_rsp_if_t apb_rsp;
        itf_read(gst_apb_rsp_slv, &apb_rsp);
        itf_write(apb_demux->host_apb_rsp_mst, &apb_rsp);
    }
}

void apb_demux_clock(apb_demux_t *apb_demux)
{
    apb_demux_proc_req(apb_demux);
    apb_demux_proc_rsp(apb_demux);
}

void apb_demux_free(apb_demux_t *apb_demux) {}
