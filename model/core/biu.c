#include "biu.h"
#include "dbg/chk.h"

#define FCH_TRANS_ID 0
#define LDST_TRANS_ID 1

void biu_construct(biu_t *biu) {}

void biu_reset(biu_t *biu) {}

void biu_free(biu_t *biu) {}

static void biu_proc_i_req(biu_t *biu)
{
    DBG_CHECK(biu->i_bti_req_mst);
    DBG_CHECK(biu->fch_req_slv);

    if (itf_fifo_full(biu->i_bti_req_mst)) {
        return;
    }

    if (itf_fifo_empty(biu->fch_req_slv)) {
        return;
    }

    fch_req_if_t fch_req;
    itf_read(biu->fch_req_slv, &fch_req);

    bti_req_if_t bti_req;
    bti_req.trans_id = FCH_TRANS_ID;
    bti_req.cmd = BTI_CMD_READ;
    bti_req.addr = fch_req.pc;
    itf_write(biu->i_bti_req_mst, &bti_req);
}

static void biu_proc_d_req(biu_t *biu)
{
    DBG_CHECK(biu->d_bti_req_mst);
    DBG_CHECK(biu->ldst_req_slv);

    if (itf_fifo_full(biu->d_bti_req_mst)) {
        return;
    }

    if (itf_fifo_empty(biu->ldst_req_slv)) {
        return;
    }

    ldst_req_if_t ldst_req;
    itf_read(biu->ldst_req_slv, &ldst_req);

    bti_req_if_t bti_req;
    bti_req.trans_id = LDST_TRANS_ID;
    bti_req.cmd = ldst_req.st ? BTI_CMD_WRITE : BTI_CMD_READ;
    bti_req.addr = ldst_req.addr;
    bti_req.data = ldst_req.data;
    bti_req.strobe = ldst_req.strobe;
    itf_write(biu->d_bti_req_mst, &bti_req);
}

static void biu_proc_i_rsp(biu_t *biu)
{
    DBG_CHECK(biu->i_bti_rsp_slv);
    DBG_CHECK(biu->fch_rsp_mst);

    if (itf_fifo_empty(biu->i_bti_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(biu->fch_rsp_mst)) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    itf_read(biu->i_bti_rsp_slv, &bti_rsp);
    DBG_CHECK(bti_rsp.trans_id == FCH_TRANS_ID);

    fch_rsp_if_t fch_rsp;
    fch_rsp.ir = bti_rsp.data;
    fch_rsp.ok = bti_rsp.ok;
    itf_write(biu->fch_rsp_mst, &fch_rsp);
}

static void biu_proc_d_rsp(biu_t *biu)
{
    DBG_CHECK(biu->d_bti_rsp_slv);
    DBG_CHECK(biu->ldst_rsp_mst);

    if (itf_fifo_empty(biu->d_bti_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(biu->ldst_rsp_mst)) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    itf_read(biu->d_bti_rsp_slv, &bti_rsp);
    DBG_CHECK(bti_rsp.trans_id == LDST_TRANS_ID);

    ldst_rsp_if_t ldst_rsp;
    ldst_rsp.data = bti_rsp.data;
    ldst_rsp.ok = bti_rsp.ok;
    itf_write(biu->ldst_rsp_mst, &ldst_rsp);
}

void biu_clock(biu_t *biu)
{
    biu_proc_i_req(biu);
    biu_proc_d_req(biu);
    biu_proc_i_rsp(biu);
    biu_proc_d_rsp(biu);
}