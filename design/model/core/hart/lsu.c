#include "lsu.h"
#include "dbg/chk.h"
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

static ldst_req_if_t lsu_make_byte_req(const lsu_t *lsu, u32 byte_idx)
{
    u32 byte = (lsu->req.data >> (byte_idx * 8u)) & 0xffu;

    ldst_req_if_t req = {
        .addr = lsu->req.addr + byte_idx,
        .st = lsu->req.st,
        .size = LDST_REQ_SIZE_B1,
        .data = byte,
        .strobe = lsu->req.st ? 0x1 : 0x0
    };
    return req;
}

static void lsu_send_rsp(lsu_t *lsu, bool ok, u32 data)
{
    if (itf_fifo_full(lsu->exu_ldst_rsp_mst)) {
        return;
    }

    ldst_rsp_if_t rsp = {
        .data = data,
        .ok = ok
    };
    itf_write(lsu->exu_ldst_rsp_mst, &rsp);
    lsu->busy = false;
    lsu->split = false;
}

static void lsu_accept_req(lsu_t *lsu)
{
    if (lsu->busy ||
        itf_fifo_empty(lsu->exu_ldst_req_slv) ||
        itf_fifo_full(lsu->hbi_ldst_req_mst)) {
        return;
    }

    ldst_req_if_t req;
    itf_read(lsu->exu_ldst_req_slv, &req);

    lsu->busy = true;
    lsu->split = lsu_translation_enabled(lsu) && lsu_cross_page(&req);
    lsu->req = req;
    lsu->byte_num = lsu_req_byte_num(req.size);
    lsu->req_byte_idx = 0;
    lsu->rsp_byte_idx = 0;
    lsu->rsp_data = 0;

    if (!lsu->split) {
        itf_write(lsu->hbi_ldst_req_mst, &req);
    }
}

static void lsu_proc_direct_rsp(lsu_t *lsu)
{
    if (!lsu->busy || lsu->split ||
        itf_fifo_empty(lsu->hbi_ldst_rsp_slv) ||
        itf_fifo_full(lsu->exu_ldst_rsp_mst)) {
        return;
    }

    ldst_rsp_if_t rsp;
    itf_read(lsu->hbi_ldst_rsp_slv, &rsp);
    itf_write(lsu->exu_ldst_rsp_mst, &rsp);
    lsu->busy = false;
}

static void lsu_proc_split_req(lsu_t *lsu)
{
    if (!lsu->busy || !lsu->split ||
        lsu->req_byte_idx >= lsu->byte_num ||
        lsu->req_byte_idx != lsu->rsp_byte_idx ||
        itf_fifo_full(lsu->hbi_ldst_req_mst)) {
        return;
    }

    ldst_req_if_t req = lsu_make_byte_req(lsu, lsu->req_byte_idx);
    itf_write(lsu->hbi_ldst_req_mst, &req);
    lsu->req_byte_idx++;
}

static void lsu_proc_split_rsp(lsu_t *lsu)
{
    if (!lsu->busy || !lsu->split ||
        lsu->rsp_byte_idx >= lsu->req_byte_idx ||
        itf_fifo_empty(lsu->hbi_ldst_rsp_slv)) {
        return;
    }

    ldst_rsp_if_t rsp;
    itf_fifo_get_front(lsu->hbi_ldst_rsp_slv, &rsp);
    if ((!rsp.ok || lsu->rsp_byte_idx + 1u == lsu->byte_num) &&
        itf_fifo_full(lsu->exu_ldst_rsp_mst)) {
        return;
    }
    itf_fifo_pop_front(lsu->hbi_ldst_rsp_slv);
    if (!rsp.ok) {
        lsu_send_rsp(lsu, false, 0);
        return;
    }

    if (!lsu->req.st) {
        lsu->rsp_data |= (rsp.data & 0xffu) << (lsu->rsp_byte_idx * 8u);
    }
    lsu->rsp_byte_idx++;

    if (lsu->rsp_byte_idx == lsu->byte_num) {
        lsu_send_rsp(lsu, true, lsu->req.st ? 0 : lsu->rsp_data);
    }
}

void lsu_construct(lsu_t *lsu, const char *parent, const char *name)
{
    mod_construct(&lsu->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    lsu->csr_state_i = itf_signal_get_src_and_chk(lsu->csr_lsu_state_in);

    dbg_vcd_add_sig("busy", DBG_SIG_TYPE_REG, 1, &lsu->busy);
    dbg_vcd_add_sig("split", DBG_SIG_TYPE_REG, 1, &lsu->split);
    dbg_vcd_add_sig("req_addr", DBG_SIG_TYPE_REG, 32, &lsu->req.addr);
    dbg_vcd_add_sig("req_st", DBG_SIG_TYPE_REG, 1, &lsu->req.st);
    dbg_vcd_add_sig("req_size", DBG_SIG_TYPE_REG, 2, &lsu->req.size);
    dbg_vcd_add_sig("byte_num", DBG_SIG_TYPE_REG, 32, &lsu->byte_num);
    dbg_vcd_add_sig("req_byte_idx", DBG_SIG_TYPE_REG, 32, &lsu->req_byte_idx);
    dbg_vcd_add_sig("rsp_byte_idx", DBG_SIG_TYPE_REG, 32, &lsu->rsp_byte_idx);
    dbg_vcd_add_sig("rsp_data", DBG_SIG_TYPE_REG, 32, &lsu->rsp_data);
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
}

void lsu_clock(lsu_t *lsu)
{
    mod_clock(&lsu->mod);
    lsu_proc_direct_rsp(lsu);
    lsu_proc_split_rsp(lsu);
    lsu_proc_split_req(lsu);
    lsu_accept_req(lsu);
}

void lsu_free(lsu_t *lsu)
{
    mod_free(&lsu->mod);
}
