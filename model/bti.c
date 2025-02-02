#include "bti.h"
#include "dbg.h"

void bti_mux_construct(bti_mux_t *bti_mux, u32 gst_num, const u32 *gst_base_addrs, const u32 *gst_sizes)
{
    bti_mux->gst_num = gst_num;
    bti_mux->gst_base_addrs = malloc(sizeof(u32) * gst_num);
    bti_mux->gst_sizes = malloc(sizeof(u32) * gst_num);
    bti_mux->gst_bti_req_msts = malloc(sizeof(itf_t *) * gst_num);
    bti_mux->gst_bti_rsp_slvs = malloc(sizeof(itf_t *) * gst_num);

    for (u32 i = 0; i < gst_num; i++) {
        bti_mux->gst_base_addrs[i] = gst_base_addrs[i];
        bti_mux->gst_sizes[i] = gst_sizes[i];
        bti_mux->gst_bti_req_msts[i] = NULL;
        bti_mux->gst_bti_rsp_slvs[i] = NULL;
    }
}

void bti_mux_reset(bti_mux_t *bti_mux) {}

static void bti_mux_proc_req(bti_mux_t *bti_mux)
{
    if (itf_fifo_empty(bti_mux->host_bti_req_slv)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_fifo_get_front(bti_mux->host_bti_req_slv, &bti_req);

    u32 gst_idx;
    for (gst_idx = 0; gst_idx < bti_mux->gst_num; gst_idx++) {
        u32 base_addr = bti_mux->gst_base_addrs[gst_idx];
        u32 size = bti_mux->gst_sizes[gst_idx];
        if (ADDR_IN(bti_req.addr, base_addr, size)) {
            break;
        }
    }
    DBG_CHECK(gst_idx < bti_mux->gst_num);

    itf_t *gst_bti_req_mst = bti_mux->gst_bti_req_msts[gst_idx];
    if (itf_fifo_full(gst_bti_req_mst)) {
        return;
    }

    itf_fifo_pop_front(bti_mux->host_bti_req_slv);
    itf_write(gst_bti_req_mst, &bti_req);
}

static void bti_mux_proc_rsp(bti_mux_t *bti_mux)
{
    for (u32 i = 0; i < bti_mux->gst_num; i++) {
        if (itf_fifo_full(bti_mux->host_bti_rsp_mst)) {
            continue;
        }

        if (itf_fifo_empty(bti_mux->gst_bti_rsp_slvs[i])) {
            continue;
        }

        bti_rsp_if_t bti_rsp;
        itf_read(bti_mux->gst_bti_rsp_slvs[i], &bti_rsp);
        itf_write(bti_mux->host_bti_rsp_mst, &bti_rsp);
    }
}

void bti_mux_clock(bti_mux_t *bti_mux)
{
    bti_mux_proc_req(bti_mux);
    bti_mux_proc_rsp(bti_mux);
}

void bti_mux_free(bti_mux_t *bti_mux)
{
    if (bti_mux->gst_base_addrs) {
        free(bti_mux->gst_base_addrs);
        bti_mux->gst_base_addrs = NULL;
    }

    if (bti_mux->gst_sizes) {
        free(bti_mux->gst_sizes);
        bti_mux->gst_sizes = NULL;
    }

    if (bti_mux->gst_bti_req_msts) {
        free(bti_mux->gst_bti_req_msts);
        bti_mux->gst_bti_req_msts = NULL;
    }

    if (bti_mux->gst_bti_rsp_slvs) {
        free(bti_mux->gst_bti_rsp_slvs);
        bti_mux->gst_bti_rsp_slvs = NULL;
    }
}
