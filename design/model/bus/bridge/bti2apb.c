#include "bti2apb.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

void bti2apb_construct(bti2apb_t *br, const char *parent, const char *name)
{
    mod_construct(&br->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    DBG_CHECK(br->bti_req_slv);
    DBG_CHECK(br->bti_rsp_mst);
    DBG_CHECK(br->apb_req_mst);
    DBG_CHECK(br->apb_rsp_slv);
}

void bti2apb_reset(bti2apb_t *br)
{
    mod_reset(&br->mod);
    br->bti_trans_id = 0;
}

static void bti2apb_proc_req(bti2apb_t *br)
{
    if (itf_fifo_empty(br->bti_req_slv)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_fifo_get_front(br->bti_req_slv, &bti_req);

    if (bti_req.cmd != BTI_REQ_CMD_READ && bti_req.cmd != BTI_REQ_CMD_WRITE) {
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
        return;
    }

    if (itf_fifo_full(br->apb_req_mst)) {
        return;
    }

    itf_fifo_pop_front(br->bti_req_slv);

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
    mod_clock(&br->mod);
    bti2apb_proc_req(br);
    bti2apb_proc_rsp(br);
}

void bti2apb_free(bti2apb_t *br)
{
    mod_free(&br->mod);
}
