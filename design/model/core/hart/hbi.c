#include "hbi.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"
#include "spec/core/hart.h"

void hbi_construct(hbi_t *hbi, const char *parent, const char *name,
    const hbi_conf_t *conf)
{
    mod_construct(&hbi->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->i_ost_depth > 0);
    DBG_CHECK(conf->d_ost_depth > 0);
    fifo_construct(&hbi->fch_req_fifo, sizeof(fch_req_if_t),
        conf->stg_fifo_depth);
    fifo_construct(&hbi->ldst_req_fifo, sizeof(ldst_req_if_t),
        conf->stg_fifo_depth);
    ostq_construct(&hbi->i_ost, sizeof(hbi_i_ost_ctx_t),
        conf->i_ost_depth);
    ostq_construct(&hbi->d_ost, sizeof(hbi_d_ost_ctx_t),
        conf->d_ost_depth);

    hbi->perf_i_stg_full = dbg_pcm_reg_perf_cnt(hbi->mod.hier_name,
        "i_stg_full");
    hbi->perf_d_stg_full = dbg_pcm_reg_perf_cnt(hbi->mod.hier_name,
        "d_stg_full");
    hbi->perf_i_ost_full = dbg_pcm_reg_perf_cnt(hbi->mod.hier_name,
        "i_ost_full");
    hbi->perf_d_ost_full = dbg_pcm_reg_perf_cnt(hbi->mod.hier_name,
        "d_ost_full");

    dbg_vcd_add_sig("fch_req_fifo_num", DBG_SIG_TYPE_REG, 32,
        &hbi->fch_req_fifo.num);
    dbg_vcd_add_sig("ldst_req_fifo_num", DBG_SIG_TYPE_REG, 32,
        &hbi->ldst_req_fifo.num);
    dbg_vcd_add_sig("i_ost_count", DBG_SIG_TYPE_REG, 32,
        &hbi->i_ost.count);
    dbg_vcd_add_sig("d_ost_count", DBG_SIG_TYPE_REG, 32,
        &hbi->d_ost.count);
}

void hbi_reset(hbi_t *hbi)
{
    mod_reset(&hbi->mod);
    fifo_reset(&hbi->fch_req_fifo);
    fifo_reset(&hbi->ldst_req_fifo);
    ostq_reset(&hbi->i_ost);
    ostq_reset(&hbi->d_ost);
    *hbi->perf_i_stg_full = 0;
    *hbi->perf_d_stg_full = 0;
    *hbi->perf_i_ost_full = 0;
    *hbi->perf_d_ost_full = 0;
}

void hbi_free(hbi_t *hbi)
{
    mod_free(&hbi->mod);
    fifo_free(&hbi->fch_req_fifo);
    fifo_free(&hbi->ldst_req_fifo);
    ostq_free(&hbi->i_ost);
    ostq_free(&hbi->d_ost);
}

static void hbi_capture_i_req(hbi_t *hbi)
{
    DBG_CHECK(hbi->fch_req_slv);

    if (fifo_full(&hbi->fch_req_fifo)) {
        if (!itf_fifo_empty(hbi->fch_req_slv)) {
            (*hbi->perf_i_stg_full)++;
        }
        return;
    }

    if (itf_fifo_empty(hbi->fch_req_slv)) {
        return;
    }

    fch_req_if_t fch_req;
    itf_read(hbi->fch_req_slv, &fch_req);
    fifo_push(&hbi->fch_req_fifo, &fch_req);
}

static void hbi_capture_d_req(hbi_t *hbi)
{
    DBG_CHECK(hbi->ldst_req_slv);

    if (fifo_full(&hbi->ldst_req_fifo)) {
        if (!itf_fifo_empty(hbi->ldst_req_slv)) {
            (*hbi->perf_d_stg_full)++;
        }
        return;
    }

    if (itf_fifo_empty(hbi->ldst_req_slv)) {
        return;
    }

    ldst_req_if_t ldst_req;
    itf_read(hbi->ldst_req_slv, &ldst_req);
    fifo_push(&hbi->ldst_req_fifo, &ldst_req);
}

static void hbi_proc_i_req(hbi_t *hbi)
{
    DBG_CHECK(hbi->i_bti_req_mst);

    if (fifo_empty(&hbi->fch_req_fifo)) {
        return;
    }

    if (ostq_full(&hbi->i_ost)) {
        (*hbi->perf_i_ost_full)++;
        return;
    }

    if (itf_fifo_full(hbi->i_bti_req_mst)) {
        return;
    }

    fch_req_if_t fch_req;
    fifo_pop(&hbi->fch_req_fifo, &fch_req);

    bti_req_if_t bti_req = {};
    bti_req.trans_id = FCH_TRANS_ID;
    bti_req.cmd = BTI_REQ_CMD_READ;
    bti_req.addr = fch_req.pc;
    bti_req.size = BTI_REQ_SIZE_B4;
    itf_write(hbi->i_bti_req_mst, &bti_req);

    hbi_i_ost_ctx_t ctx = {
        .pc = fch_req.pc,
        .rsp_vld = false,
        .rsp = {}
    };
    bool alloc_ok = ostq_alloc(&hbi->i_ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);
}

static void hbi_proc_d_req(hbi_t *hbi)
{
    DBG_CHECK(hbi->d_bti_req_mst);

    if (fifo_empty(&hbi->ldst_req_fifo)) {
        return;
    }

    if (ostq_full(&hbi->d_ost)) {
        (*hbi->perf_d_ost_full)++;
        return;
    }

    if (itf_fifo_full(hbi->d_bti_req_mst)) {
        return;
    }

    ldst_req_if_t ldst_req;
    fifo_pop(&hbi->ldst_req_fifo, &ldst_req);

    bti_req_if_t bti_req = {};
    bti_req.trans_id = LDST_TRANS_ID;
    switch (ldst_req.cmo) {
    case LDST_REQ_CMO_NONE:
        bti_req.cmd = ldst_req.st ? BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
        break;
    case LDST_REQ_CMO_INVAL:
        bti_req.cmd = BTI_REQ_CMD_CBO_INVAL;
        break;
    case LDST_REQ_CMO_CLEAN:
        bti_req.cmd = BTI_REQ_CMD_CBO_CLEAN;
        break;
    case LDST_REQ_CMO_FLUSH:
        bti_req.cmd = BTI_REQ_CMD_CBO_FLUSH;
        break;
    default:
        DBG_CHECK(0);
        return;
    }
    bti_req.addr = ldst_req.addr;
    bti_req.size = (bti_req_size_t)ldst_req.size;
    bti_req.data = ldst_req.data;
    bti_req.strobe = ldst_req.strobe;
    itf_write(hbi->d_bti_req_mst, &bti_req);

    hbi_d_ost_ctx_t ctx = {
        .rsp_vld = false,
        .rsp = {}
    };
    bool alloc_ok = ostq_alloc(&hbi->d_ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);
}

static bool hbi_find_i_slot_by_pc(hbi_t *hbi, u32 pc, hbi_i_ost_ctx_t *ctx,
    u32 *slot)
{
    for (u32 i = 0; i < hbi->i_ost.depth; i++) {
        u32 idx = (hbi->i_ost.rptr + i) % hbi->i_ost.depth;
        if (!ostq_slot_valid(&hbi->i_ost, idx)) {
            continue;
        }

        hbi_i_ost_ctx_t cur;
        ostq_read_slot(&hbi->i_ost, idx, &cur);
        if (cur.pc != pc) {
            continue;
        }

        if (ctx != NULL) {
            *ctx = cur;
        }
        if (slot != NULL) {
            *slot = idx;
        }
        return true;
    }

    return false;
}

static bool hbi_find_oldest_i_wait_slot(hbi_t *hbi, hbi_i_ost_ctx_t *ctx,
    u32 *slot)
{
    for (u32 i = 0; i < hbi->i_ost.depth; i++) {
        u32 idx = (hbi->i_ost.rptr + i) % hbi->i_ost.depth;
        if (!ostq_slot_valid(&hbi->i_ost, idx)) {
            continue;
        }

        hbi_i_ost_ctx_t cur;
        ostq_read_slot(&hbi->i_ost, idx, &cur);
        if (cur.rsp_vld) {
            continue;
        }

        if (ctx != NULL) {
            *ctx = cur;
        }
        if (slot != NULL) {
            *slot = idx;
        }
        return true;
    }

    return false;
}

static bool hbi_find_oldest_d_wait_slot(hbi_t *hbi, hbi_d_ost_ctx_t *ctx,
    u32 *slot)
{
    for (u32 i = 0; i < hbi->d_ost.depth; i++) {
        u32 idx = (hbi->d_ost.rptr + i) % hbi->d_ost.depth;
        if (!ostq_slot_valid(&hbi->d_ost, idx)) {
            continue;
        }

        hbi_d_ost_ctx_t cur;
        ostq_read_slot(&hbi->d_ost, idx, &cur);
        if (cur.rsp_vld) {
            continue;
        }

        if (ctx != NULL) {
            *ctx = cur;
        }
        if (slot != NULL) {
            *slot = idx;
        }
        return true;
    }

    return false;
}

static void hbi_proc_i_rsp(hbi_t *hbi)
{
    DBG_CHECK(hbi->i_bti_rsp_slv);

    if (itf_fifo_empty(hbi->i_bti_rsp_slv)) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    itf_read(hbi->i_bti_rsp_slv, &bti_rsp);
    DBG_CHECK(bti_rsp.trans_id == FCH_TRANS_ID);

    hbi_i_ost_ctx_t ctx;
    u32 slot;
    bool found = hbi_find_oldest_i_wait_slot(hbi, &ctx, &slot);
    DBG_CHECK(found);
    DBG_CHECK(!ctx.rsp_vld);

    ctx.rsp.ir = bti_rsp.data;
    ctx.rsp.ok = bti_rsp.ok;
    ctx.rsp.expt = false;
    ctx.rsp.cause = 0;
    ctx.rsp.priv = 0;
    ctx.rsp.tval = 0;
    ctx.rsp_vld = true;
    ostq_write_slot(&hbi->i_ost, slot, &ctx);
}

static void hbi_proc_i_expt(hbi_t *hbi)
{
    if (itf_fifo_empty(hbi->mmu_fch_expt_slv)) {
        return;
    }

    hart_expt_if_t expt;
    itf_fifo_get_front(hbi->mmu_fch_expt_slv, &expt);
    DBG_CHECK(expt.type == HART_EXPT_TYPE_EXCEPTION);

    hbi_i_ost_ctx_t ctx;
    u32 slot;
    bool found = hbi_find_i_slot_by_pc(hbi, expt.pc, &ctx, &slot);
    DBG_CHECK(found);
    DBG_CHECK(!ctx.rsp_vld);

    itf_fifo_pop_front(hbi->mmu_fch_expt_slv);

    ctx.rsp.ir = 0;
    ctx.rsp.ok = false;
    ctx.rsp.expt = true;
    ctx.rsp.cause = (u32)expt.cause;
    ctx.rsp.priv = expt.priv;
    ctx.rsp.tval = expt.tval;
    ctx.rsp_vld = true;
    ostq_write_slot(&hbi->i_ost, slot, &ctx);
}

static void hbi_send_i_rsp(hbi_t *hbi)
{
    DBG_CHECK(hbi->fch_rsp_mst);

    if (itf_fifo_full(hbi->fch_rsp_mst)) {
        return;
    }

    hbi_i_ost_ctx_t ctx;
    bool found = ostq_peek_head(&hbi->i_ost, &ctx, NULL);
    if (!found || !ctx.rsp_vld) {
        return;
    }

    itf_write(hbi->fch_rsp_mst, &ctx.rsp);
    ostq_free_head(&hbi->i_ost);
}

static void hbi_proc_d_rsp(hbi_t *hbi)
{
    DBG_CHECK(hbi->d_bti_rsp_slv);

    if (itf_fifo_empty(hbi->d_bti_rsp_slv)) {
        return;
    }

    hbi_d_ost_ctx_t ctx;
    u32 slot;
    bool found = hbi_find_oldest_d_wait_slot(hbi, &ctx, &slot);
    DBG_CHECK(found);
    DBG_CHECK(!ctx.rsp_vld);

    bti_rsp_if_t bti_rsp;
    itf_read(hbi->d_bti_rsp_slv, &bti_rsp);
    DBG_CHECK(bti_rsp.trans_id == LDST_TRANS_ID);

    ctx.rsp.data = bti_rsp.data;
    ctx.rsp.ok = bti_rsp.ok;
    ctx.rsp_vld = true;
    ostq_write_slot(&hbi->d_ost, slot, &ctx);
}

static void hbi_send_d_rsp(hbi_t *hbi)
{
    DBG_CHECK(hbi->ldst_rsp_mst);

    if (itf_fifo_full(hbi->ldst_rsp_mst)) {
        return;
    }

    hbi_d_ost_ctx_t ctx;
    bool found = ostq_peek_head(&hbi->d_ost, &ctx, NULL);
    if (!found || !ctx.rsp_vld) {
        return;
    }

    itf_write(hbi->ldst_rsp_mst, &ctx.rsp);
    ostq_free_head(&hbi->d_ost);
}

void hbi_clock(hbi_t *hbi)
{
    mod_clock(&hbi->mod);
    ostq_clock(&hbi->i_ost);
    ostq_clock(&hbi->d_ost);
    hbi_capture_i_req(hbi);
    hbi_capture_d_req(hbi);
    hbi_proc_i_expt(hbi);
    hbi_proc_i_rsp(hbi);
    hbi_send_i_rsp(hbi);
    hbi_proc_d_rsp(hbi);
    hbi_send_d_rsp(hbi);
    hbi_proc_i_req(hbi);
    hbi_proc_d_req(hbi);
}
