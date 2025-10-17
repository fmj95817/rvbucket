#include "mmu.h"
#include "base/def.h"
#include "dbg/chk.h"

void mmu_construct(mmu_t *mmu, const mmu_conf_t *conf)
{

}

void mmu_reset(mmu_t *mmu)
{

}

static void mmu_proc_i_req(mmu_t *mmu)
{
    DBG_CHECK(mmu->pa_i_bti_req_mst);
    DBG_CHECK(mmu->va_i_bti_req_slv);

    if (itf_fifo_full(mmu->pa_i_bti_req_mst)) {
        return;
    }

    if (itf_fifo_empty(mmu->va_i_bti_req_slv)) {
        return;
    }

    bti_req_if_t va_i_bti_req;
    itf_read(mmu->va_i_bti_req_slv, &va_i_bti_req);

    bti_req_if_t pa_i_bti_req;
    memcpy(&pa_i_bti_req, &va_i_bti_req, sizeof(bti_req_if_t));

    itf_write(mmu->pa_i_bti_req_mst, &pa_i_bti_req);
}

static void mmu_proc_d_req(mmu_t *mmu)
{
    DBG_CHECK(mmu->pa_d_bti_req_mst);
    DBG_CHECK(mmu->va_d_bti_req_slv);

    if (itf_fifo_full(mmu->pa_d_bti_req_mst)) {
        return;
    }

    if (itf_fifo_empty(mmu->va_d_bti_req_slv)) {
        return;
    }

    bti_req_if_t va_d_bti_req;
    itf_read(mmu->va_d_bti_req_slv, &va_d_bti_req);

    bti_req_if_t pa_d_bti_req;
    memcpy(&pa_d_bti_req, &va_d_bti_req, sizeof(bti_req_if_t));

    itf_write(mmu->pa_d_bti_req_mst, &pa_d_bti_req);
}

static void mmu_proc_i_rsp(mmu_t *mmu)
{
    DBG_CHECK(mmu->pa_i_bti_rsp_slv);
    DBG_CHECK(mmu->va_i_bti_rsp_mst);

    if (itf_fifo_empty(mmu->pa_i_bti_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(mmu->va_i_bti_rsp_mst)) {
        return;
    }

    bti_rsp_if_t pa_i_bti_rsp;
    itf_read(mmu->pa_i_bti_rsp_slv, &pa_i_bti_rsp);

    bti_rsp_if_t va_i_bti_rsp;
    memcpy(&va_i_bti_rsp, &pa_i_bti_rsp, sizeof(bti_rsp_if_t));

    itf_write(mmu->va_i_bti_rsp_mst, &va_i_bti_rsp);
}

static void mmu_proc_d_rsp(mmu_t *mmu)
{
    DBG_CHECK(mmu->pa_d_bti_rsp_slv);
    DBG_CHECK(mmu->va_d_bti_rsp_mst);

    if (itf_fifo_empty(mmu->pa_d_bti_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(mmu->va_d_bti_rsp_mst)) {
        return;
    }

    bti_rsp_if_t pa_d_bti_rsp;
    itf_read(mmu->pa_d_bti_rsp_slv, &pa_d_bti_rsp);

    bti_rsp_if_t va_d_bti_rsp;
    memcpy(&va_d_bti_rsp, &pa_d_bti_rsp, sizeof(bti_rsp_if_t));

    itf_write(mmu->va_d_bti_rsp_mst, &va_d_bti_rsp);
}

void mmu_clock(mmu_t *mmu)
{
    mmu_proc_i_req(mmu);
    mmu_proc_i_rsp(mmu);
    mmu_proc_d_req(mmu);
    mmu_proc_d_rsp(mmu);
}

void mmu_free(mmu_t *mmu)
{

}
