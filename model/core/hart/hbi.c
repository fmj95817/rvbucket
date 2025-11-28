#include "hbi.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "spec/hart.h"

void hbi_construct(hbi_t *hbi, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);
}

void hbi_reset(hbi_t *hbi) {}

void hbi_free(hbi_t *hbi) {}

static void hbi_proc_i_req(hbi_t *hbi)
{
    DBG_CHECK(hbi->i_bti_req_mst);
    DBG_CHECK(hbi->fch_req_slv);

    if (itf_fifo_full(hbi->i_bti_req_mst)) {
        return;
    }

    if (itf_fifo_empty(hbi->fch_req_slv)) {
        return;
    }

    fch_req_if_t fch_req;
    itf_read(hbi->fch_req_slv, &fch_req);

    bti_req_if_t bti_req;
    bti_req.trans_id = FCH_TRANS_ID;
    bti_req.cmd = BTI_REQ_CMD_READ;
    bti_req.addr = fch_req.pc;
    itf_write(hbi->i_bti_req_mst, &bti_req);
}

static void hbi_proc_d_req(hbi_t *hbi)
{
    DBG_CHECK(hbi->d_bti_req_mst);
    DBG_CHECK(hbi->ldst_req_slv);

    if (itf_fifo_full(hbi->d_bti_req_mst)) {
        return;
    }

    if (itf_fifo_empty(hbi->ldst_req_slv)) {
        return;
    }

    ldst_req_if_t ldst_req;
    itf_read(hbi->ldst_req_slv, &ldst_req);

    bti_req_if_t bti_req;
    bti_req.trans_id = LDST_TRANS_ID;
    bti_req.cmd = ldst_req.st ? BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
    bti_req.addr = ldst_req.addr;
    bti_req.data = ldst_req.data;
    bti_req.strobe = ldst_req.strobe;
    itf_write(hbi->d_bti_req_mst, &bti_req);
}

static void hbi_proc_i_rsp(hbi_t *hbi)
{
    DBG_CHECK(hbi->i_bti_rsp_slv);
    DBG_CHECK(hbi->fch_rsp_mst);

    if (itf_fifo_empty(hbi->i_bti_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(hbi->fch_rsp_mst)) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    itf_read(hbi->i_bti_rsp_slv, &bti_rsp);
    DBG_CHECK(bti_rsp.trans_id == FCH_TRANS_ID);

    fch_rsp_if_t fch_rsp;
    fch_rsp.ir = bti_rsp.data;
    fch_rsp.ok = bti_rsp.ok;
    itf_write(hbi->fch_rsp_mst, &fch_rsp);
}

static void hbi_proc_d_rsp(hbi_t *hbi)
{
    DBG_CHECK(hbi->d_bti_rsp_slv);
    DBG_CHECK(hbi->ldst_rsp_mst);

    if (itf_fifo_empty(hbi->d_bti_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(hbi->ldst_rsp_mst)) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    itf_read(hbi->d_bti_rsp_slv, &bti_rsp);
    DBG_CHECK(bti_rsp.trans_id == LDST_TRANS_ID);

    ldst_rsp_if_t ldst_rsp;
    ldst_rsp.data = bti_rsp.data;
    ldst_rsp.ok = bti_rsp.ok;
    itf_write(hbi->ldst_rsp_mst, &ldst_rsp);
}

void hbi_clock(hbi_t *hbi)
{
    hbi_proc_i_req(hbi);
    hbi_proc_d_req(hbi);
    hbi_proc_i_rsp(hbi);
    hbi_proc_d_rsp(hbi);
}