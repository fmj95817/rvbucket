#include "mux.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void bti_mux_construct(bti_mux_t *bti_mux, const char *name, u32 host_num)
{
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(host_num <= BTI_MUX_HOST_NUM_MAX);
    bti_mux->host_num = host_num;
}

void bti_mux_reset(bti_mux_t *bti_mux)
{
    bti_mux->rsp_pending = false;
    bti_mux->rsp_host_idx = 0;
    bti_mux->req_rr_idx = 0;
}

static void bti_mux_proc_req(bti_mux_t *bti_mux)
{
    if (bti_mux->rsp_pending) {
        return;
    }
    if (itf_fifo_full(bti_mux->gst_bti_req_mst)) {
        return;
    }

    for (u32 i = 0; i < bti_mux->host_num; i++) {
        u32 host_idx = (bti_mux->req_rr_idx + i) % bti_mux->host_num;
        itf_t *host_req = bti_mux->host_bti_req_slvs[host_idx];
        if (host_req == NULL || itf_fifo_empty(host_req)) {
            continue;
        }

        bti_req_if_t req;
        itf_read(host_req, &req);
        itf_write(bti_mux->gst_bti_req_mst, &req);
        bti_mux->rsp_pending = true;
        bti_mux->rsp_host_idx = host_idx;
        bti_mux->req_rr_idx = (host_idx + 1) % bti_mux->host_num;
        return;
    }
}

static void bti_mux_proc_rsp(bti_mux_t *bti_mux)
{
    if (!bti_mux->rsp_pending) {
        return;
    }
    if (itf_fifo_empty(bti_mux->gst_bti_rsp_slv)) {
        return;
    }

    itf_t *host_rsp = bti_mux->host_bti_rsp_msts[bti_mux->rsp_host_idx];
    DBG_CHECK(host_rsp != NULL);
    if (itf_fifo_full(host_rsp)) {
        return;
    }

    bti_rsp_if_t rsp;
    itf_read(bti_mux->gst_bti_rsp_slv, &rsp);
    itf_write(host_rsp, &rsp);
    bti_mux->rsp_pending = false;
}

void bti_mux_clock(bti_mux_t *bti_mux)
{
    bti_mux_proc_rsp(bti_mux);
    bti_mux_proc_req(bti_mux);
}

void bti_mux_free(bti_mux_t *bti_mux)
{
    (void)bti_mux;
}
