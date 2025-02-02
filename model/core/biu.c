#include "biu.h"
#include "mod_if.h"
#include "bti.h"
#include "dbg.h"

#define IFETCH_TRANS_ID 0
#define LDST_TRANS_ID 1

void biu_construct(biu_t *biu)
{
    arb_construct(&biu->rr_arb, 2);
}

void biu_reset(biu_t *biu)
{
    arb_reset(&biu->rr_arb);
}

void biu_free(biu_t *biu)
{
    arb_free(&biu->rr_arb);
}

static void biu_proc_req(biu_t *biu)
{
    DBG_CHECK(biu->bti_req_mst);
    DBG_CHECK(biu->fch_req_slv);
    DBG_CHECK(biu->ldst_req_slv);

    if (itf_fifo_full(biu->bti_req_mst)) {
        return;
    }

    DBG_CHECK(biu->rr_arb.req);

    biu->rr_arb.req[0] = !itf_fifo_empty(biu->fch_req_slv);
    biu->rr_arb.req[1] = !itf_fifo_empty(biu->ldst_req_slv);
    arb_clock(&biu->rr_arb);

    if (biu->rr_arb.grant_idx == 0) {
        fch_req_if_t fch_req;
        itf_read(biu->fch_req_slv, &fch_req);

        bti_req_if_t bti_req;
        bti_req.trans_id = IFETCH_TRANS_ID;
        bti_req.cmd = BTI_CMD_READ;
        bti_req.addr = fch_req.pc;
        itf_write(biu->bti_req_mst, &bti_req);
    }

    if (biu->rr_arb.grant_idx == 1) {
        ldst_req_if_t ldst_req;
        itf_read(biu->ldst_req_slv, &ldst_req);

        bti_req_if_t bti_req;
        bti_req.trans_id = LDST_TRANS_ID;
        bti_req.cmd = ldst_req.st ? BTI_CMD_WRITE : BTI_CMD_READ;
        bti_req.addr = ldst_req.addr;
        bti_req.data = ldst_req.data;
        bti_req.strobe = ldst_req.strobe;
        itf_write(biu->bti_req_mst, &bti_req);
    }
}

static void biu_proc_rsp(biu_t *biu)
{
    DBG_CHECK(biu->bti_rsp_slv);
    DBG_CHECK(biu->fch_rsp_mst);
    DBG_CHECK(biu->ldst_rsp_mst);

    if (itf_fifo_empty(biu->bti_rsp_slv)) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    itf_fifo_get_front(biu->bti_rsp_slv, &bti_rsp);

    if ((bti_rsp.trans_id == IFETCH_TRANS_ID) && !itf_fifo_full(biu->fch_rsp_mst)) {
        fch_rsp_if_t fch_rsp;
        fch_rsp.ir = bti_rsp.data;
        fch_rsp.ok = bti_rsp.ok;
        itf_write(biu->fch_rsp_mst, &fch_rsp);
        itf_fifo_pop_front(biu->bti_rsp_slv);
    }

    if ((bti_rsp.trans_id == LDST_TRANS_ID) && !itf_fifo_full(biu->ldst_rsp_mst)) {
        ldst_rsp_if_t ldst_rsp;
        ldst_rsp.data = bti_rsp.data;
        ldst_rsp.ok = bti_rsp.ok;
        itf_write(biu->ldst_rsp_mst, &ldst_rsp);
        itf_fifo_pop_front(biu->bti_rsp_slv);
    }
}

void biu_clock(biu_t *biu)
{
    biu_proc_req(biu);
    biu_proc_rsp(biu);
}