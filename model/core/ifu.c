#include "ifu.h"
#include "mod_if.h"
#include "dbg.h"

void ifu_construct(ifu_t *ifu, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size)
{
    ifu->reset_pc = reset_pc;
    ifu->boot_rom_info.base = boot_rom_base;
    ifu->boot_rom_info.size = boot_rom_size;
}

void ifu_reset(ifu_t *ifu)
{
    ifu->pc = ifu->reset_pc;
    ifu->fl_req_pend = false;
    ifu->fch_req_pend = false;
}

void ifu_free(ifu_t *ifu) {}


static void ifu_send_fetch(ifu_t *ifu)
{
    if (ifu->fl_req_pend) {
        return;
    }

    if (itf_fifo_full(ifu->fch_req_mst)) {
        return;
    }

    if (ifu->fch_req_pend) {
        return;
    }

    fch_req_if_t fch_req;
    fch_req.pc = ifu->pc;
    itf_write(ifu->fch_req_mst, &fch_req);

    ifu->fch_req_pend = true;
}

static void ifu_update_pc(ifu_t *ifu)
{
    if (ifu->fch_req_pend) {
        return;
    }

    if (ifu->fl_req_pend) {
        return;
    }

    u32 pc_nxt = ifu->pc + 4;

    if (!itf_fifo_empty(ifu->ex_rsp_slv)) {
        ex_rsp_if_t ex_rsp;
        itf_read(ifu->ex_rsp_slv, &ex_rsp);

        if (ex_rsp.taken) {
            pc_nxt = ex_rsp.target_pc;
            fl_req_if_t fl_req;
            itf_write(ifu->fl_req_mst, &fl_req);
            ifu->fl_req_pend = true;
        }
    }

    ifu->pc = pc_nxt;
}

static void ifu_proc_fl_rsp(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->fl_rsp_slv)) {
        return;
    }

    fl_rsp_if_t fl_rsp;
    itf_read(ifu->fl_rsp_slv, &fl_rsp);
    ifu->fl_req_pend = false;
}

static void ifu_proc_fch_rsp(ifu_t *ifu)
{
    if (itf_fifo_empty(ifu->fch_rsp_slv)) {
        return;
    }

    if (itf_fifo_full(ifu->ex_req_mst)) {
        return;
    }

    fch_rsp_if_t fch_rsp;
    itf_read(ifu->fch_rsp_slv, &fch_rsp);
    ifu->fch_req_pend = false;

    ex_req_if_t ex_req;
    ex_req.inst.raw = fch_rsp.ir;
    ex_req.pc = ifu->pc;
    ex_req.is_boot_code = ADDR_IN(ifu->pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);
    itf_write(ifu->ex_req_mst, &ex_req);

    ifu_update_pc(ifu);
}

void ifu_clock(ifu_t *ifu)
{
    ifu_send_fetch(ifu);
    ifu_proc_fl_rsp(ifu);
    ifu_proc_fch_rsp(ifu);
}