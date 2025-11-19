#include "bridge.h"
#include "dbg/chk.h"

void bti2apb_construct(bti2apb_t *br)
{
    DBG_CHECK(br->bti_req_slv);
    DBG_CHECK(br->bti_rsp_mst);
    DBG_CHECK(br->apb_req_mst);
    DBG_CHECK(br->apb_rsp_slv);
}

void bti2apb_reset(bti2apb_t *br)
{
    br->bti_trans_id = 0;
}

static void bti2apb_proc_req(bti2apb_t *br)
{
    if (itf_fifo_empty(br->bti_req_slv)) {
        return;
    }

    if (itf_fifo_full(br->apb_req_mst)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(br->bti_req_slv, &bti_req);

    apb_req_if_t apb_req = {
        .paddr = bti_req.addr,
        .pwrite = (bti_req.cmd == BTI_REQ_CMD_WRITE),
        .pwdata = bti_req.data,
        .pstrb = bti_req.strobe
    };
    itf_write(br->apb_req_mst, &apb_req);
    br->bti_trans_id = bti_req.trans_id;
}

static void bti2apb_proc_rsp(bti2apb_t *br)
{
    if (itf_fifo_empty(br->apb_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(br->bti_rsp_mst)) {
        return;
    }

    apb_rsp_if_t apb_rsp;
    itf_read(br->apb_rsp_slv, &apb_rsp);

    bti_rsp_if_t bti_rsp = {
        .data = apb_rsp.prdata,
        .ok = !apb_rsp.pslverr,
        .trans_id = br->bti_trans_id
    };
    itf_write(br->bti_rsp_mst, &bti_rsp);
}

void bti2apb_clock(bti2apb_t *br)
{
    bti2apb_proc_req(br);
    bti2apb_proc_rsp(br);
}

void bti2apb_free(bti2apb_t *br) {}
