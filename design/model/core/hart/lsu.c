#include "lsu.h"
#include "dbg/chk.h"
#include "dbg/pcm.h"
#include "dbg/vcd.h"
#include "spec/core/csr.h"

#define LSU_PGSHIFT 12u
#define LSU_PGSIZE (1u << LSU_PGSHIFT)
#define LSU_PGMASK (LSU_PGSIZE - 1u)

static u32 lsu_req_byte_num(ldst_req_size_t size)
{
    DBG_CHECK(size <= LDST_REQ_SIZE_B4);
    return 1u << size;
}

static bool lsu_translation_enabled(const lsu_t *lsu)
{
    return (lsu->csr_state_i->satp & RV32G_CSR_SATP_MODE_BIT) != 0;
}

static bool lsu_cross_page(const ldst_req_if_t *req)
{
    u32 byte_num = lsu_req_byte_num(req->size);
    return (req->addr & LSU_PGMASK) + byte_num > LSU_PGSIZE;
}

static ldst_req_if_t lsu_make_byte_req(const ldst_req_if_t *src, u32 byte_idx)
{
    u32 byte = (src->data >> (byte_idx * 8u)) & 0xffu;

    ldst_req_if_t req = {
        .addr = src->addr + byte_idx,
        .st = src->st,
        .size = LDST_REQ_SIZE_B1,
        .data = byte,
        .strobe = src->st && (src->strobe & (1u << byte_idx)) ? 0x1 : 0x0
    };
    return req;
}

static void lsu_capture_req(lsu_t *lsu)
{
    if (fifo_full(&lsu->req_fifo)) {
        if (!itf_fifo_empty(lsu->exu_ldst_req_slv)) {
            (*lsu->perf_stg_full)++;
        }
        return;
    }

    if (itf_fifo_empty(lsu->exu_ldst_req_slv)) {
        return;
    }

    ldst_req_if_t req;
    itf_read(lsu->exu_ldst_req_slv, &req);
    fifo_push(&lsu->req_fifo, &req);
}

static void lsu_alloc_req(lsu_t *lsu)
{
    if (fifo_empty(&lsu->req_fifo)) {
        return;
    }
    if (ostq_full(&lsu->ost)) {
        (*lsu->perf_ost_full)++;
        return;
    }

    ldst_req_if_t req;
    fifo_pop(&lsu->req_fifo, &req);

    bool split = lsu_translation_enabled(lsu) && lsu_cross_page(&req);
    lsu_ost_ctx_t ctx = {
        .req = req,
        .split = split,
        .byte_num = split ? lsu_req_byte_num(req.size) : 1,
        .req_idx = 0,
        .rsp_idx = 0,
        .rsp_data = 0,
        .ready = false,
        .ok = true
    };
    bool alloc_ok = ostq_alloc(&lsu->ost, &ctx, NULL);
    DBG_CHECK(alloc_ok);
}

static bool lsu_find_issue_slot(lsu_t *lsu, lsu_ost_ctx_t *ctx, u32 *slot)
{
    for (u32 i = 0; i < lsu->ost.depth; i++) {
        u32 idx = (lsu->ost.rptr + i) % lsu->ost.depth;
        if (!ostq_slot_valid(&lsu->ost, idx)) {
            continue;
        }

        lsu_ost_ctx_t cur;
        ostq_read_slot(&lsu->ost, idx, &cur);
        if (cur.ready || cur.req_idx >= cur.byte_num) {
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

static bool lsu_find_rsp_slot(lsu_t *lsu, lsu_ost_ctx_t *ctx, u32 *slot)
{
    for (u32 i = 0; i < lsu->ost.depth; i++) {
        u32 idx = (lsu->ost.rptr + i) % lsu->ost.depth;
        if (!ostq_slot_valid(&lsu->ost, idx)) {
            continue;
        }

        lsu_ost_ctx_t cur;
        ostq_read_slot(&lsu->ost, idx, &cur);
        if (cur.ready || cur.rsp_idx >= cur.req_idx) {
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

static void lsu_issue_req(lsu_t *lsu)
{
    if (itf_fifo_full(lsu->hbi_ldst_req_mst)) {
        return;
    }

    lsu_ost_ctx_t ctx;
    u32 slot;
    if (!lsu_find_issue_slot(lsu, &ctx, &slot)) {
        return;
    }

    ldst_req_if_t req = ctx.split ?
        lsu_make_byte_req(&ctx.req, ctx.req_idx) : ctx.req;
    itf_write(lsu->hbi_ldst_req_mst, &req);
    ctx.req_idx++;
    ostq_write_slot(&lsu->ost, slot, &ctx);
}

static void lsu_proc_rsp(lsu_t *lsu)
{
    if (itf_fifo_empty(lsu->hbi_ldst_rsp_slv)) {
        return;
    }

    lsu_ost_ctx_t ctx;
    u32 slot;
    if (!lsu_find_rsp_slot(lsu, &ctx, &slot)) {
        return;
    }

    ldst_rsp_if_t rsp;
    itf_read(lsu->hbi_ldst_rsp_slv, &rsp);

    if (!rsp.ok) {
        ctx.ok = false;
        ctx.ready = true;
        ostq_write_slot(&lsu->ost, slot, &ctx);
        return;
    }

    if (ctx.split) {
        if (!ctx.req.st) {
            ctx.rsp_data |= (rsp.data & 0xffu) << (ctx.rsp_idx * 8u);
        }
    } else {
        ctx.rsp_data = rsp.data;
    }
    ctx.rsp_idx++;

    if (ctx.rsp_idx == ctx.byte_num) {
        ctx.ready = true;
        ctx.ok = true;
    }
    ostq_write_slot(&lsu->ost, slot, &ctx);
}

static void lsu_send_ready_rsp(lsu_t *lsu)
{
    if (itf_fifo_full(lsu->exu_ldst_rsp_mst)) {
        return;
    }

    lsu_ost_ctx_t ctx;
    bool found = ostq_peek_head(&lsu->ost, &ctx, NULL);
    if (!found || !ctx.ready) {
        return;
    }

    ldst_rsp_if_t rsp = {
        .data = ctx.ok && !ctx.req.st ? ctx.rsp_data : 0,
        .ok = ctx.ok
    };
    itf_write(lsu->exu_ldst_rsp_mst, &rsp);
    ostq_free_head(&lsu->ost);
}

static void lsu_update_dbg_state(lsu_t *lsu)
{
    lsu->busy = !ostq_empty(&lsu->ost) || !fifo_empty(&lsu->req_fifo);
    lsu->split = false;
    lsu->req = (ldst_req_if_t){};
    lsu->byte_num = 0;
    lsu->req_byte_idx = 0;
    lsu->rsp_byte_idx = 0;
    lsu->rsp_data = 0;

    lsu_ost_ctx_t ctx;
    if (ostq_peek_head(&lsu->ost, &ctx, NULL)) {
        lsu->split = ctx.split;
        lsu->req = ctx.req;
        lsu->byte_num = ctx.byte_num;
        lsu->req_byte_idx = ctx.req_idx;
        lsu->rsp_byte_idx = ctx.rsp_idx;
        lsu->rsp_data = ctx.rsp_data;
    }
}

void lsu_construct(lsu_t *lsu, const char *parent, const char *name,
    const lsu_conf_t *conf)
{
    mod_construct(&lsu->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf);
    DBG_CHECK(conf->stg_fifo_depth > 0);
    DBG_CHECK(conf->ost_depth > 0);

    lsu->csr_state_i = itf_signal_get_src_and_chk(lsu->csr_lsu_state_in);
    fifo_construct(&lsu->req_fifo, sizeof(ldst_req_if_t),
        conf->stg_fifo_depth);
    ostq_construct(&lsu->ost, sizeof(lsu_ost_ctx_t), conf->ost_depth);

    lsu->perf_stg_full = dbg_pcm_reg_perf_cnt(lsu->mod.hier_name,
        "stg_full");
    lsu->perf_ost_full = dbg_pcm_reg_perf_cnt(lsu->mod.hier_name,
        "ost_full");

    dbg_vcd_add_sig("busy", DBG_SIG_TYPE_REG, 1, &lsu->busy);
    dbg_vcd_add_sig("split", DBG_SIG_TYPE_REG, 1, &lsu->split);
    dbg_vcd_add_sig("req_addr", DBG_SIG_TYPE_REG, 32, &lsu->req.addr);
    dbg_vcd_add_sig("req_st", DBG_SIG_TYPE_REG, 1, &lsu->req.st);
    dbg_vcd_add_sig("req_size", DBG_SIG_TYPE_REG, 2, &lsu->req.size);
    dbg_vcd_add_sig("byte_num", DBG_SIG_TYPE_REG, 32, &lsu->byte_num);
    dbg_vcd_add_sig("req_byte_idx", DBG_SIG_TYPE_REG, 32, &lsu->req_byte_idx);
    dbg_vcd_add_sig("rsp_byte_idx", DBG_SIG_TYPE_REG, 32, &lsu->rsp_byte_idx);
    dbg_vcd_add_sig("rsp_data", DBG_SIG_TYPE_REG, 32, &lsu->rsp_data);
    dbg_vcd_add_sig("req_fifo_num", DBG_SIG_TYPE_REG, 32, &lsu->req_fifo.num);
    dbg_vcd_add_sig("ost_count", DBG_SIG_TYPE_REG, 32, &lsu->ost.count);
}

void lsu_reset(lsu_t *lsu)
{
    mod_reset(&lsu->mod);
    lsu->busy = false;
    lsu->split = false;
    lsu->req = (ldst_req_if_t){};
    lsu->byte_num = 0;
    lsu->req_byte_idx = 0;
    lsu->rsp_byte_idx = 0;
    lsu->rsp_data = 0;
    fifo_reset(&lsu->req_fifo);
    ostq_reset(&lsu->ost);
    *lsu->perf_stg_full = 0;
    *lsu->perf_ost_full = 0;
}

void lsu_clock(lsu_t *lsu)
{
    mod_clock(&lsu->mod);
    ostq_clock(&lsu->ost);
    lsu_capture_req(lsu);
    lsu_proc_rsp(lsu);
    lsu_send_ready_rsp(lsu);
    lsu_issue_req(lsu);
    lsu_alloc_req(lsu);
    lsu_issue_req(lsu);
    lsu_update_dbg_state(lsu);
}

void lsu_free(lsu_t *lsu)
{
    mod_free(&lsu->mod);
    fifo_free(&lsu->req_fifo);
    ostq_free(&lsu->ost);
}
