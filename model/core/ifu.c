#include "ifu.h"
#include "biu.h"
#include "mod_if.h"
#include "dbg.h"

void ifu_construct(ifu_t *ifu, biu_t *biu, u32 reset_pc, u32 boot_rom_base, u32 boot_rom_size)
{
    ifu->biu = biu;
    ifu->reset_pc = reset_pc;

    ifu->boot_rom_info.base = boot_rom_base;
    ifu->boot_rom_info.size = boot_rom_size;
}

void ifu_reset(ifu_t *ifu)
{
    ifu->pc = ifu->reset_pc;
}

void ifu_free(ifu_t *ifu) {}

void ifu_exec(ifu_t *ifu, iexec_if_t *i)
{
    DBG_CHECK(ifu->biu);

    ifetch_if_t ifetch_if;
    ifetch_if.req.pc = ifu->pc;

    biu_ifetch_trans_handler(ifu->biu, &ifetch_if);
    DBG_CHECK(ifetch_if.rsp.ok);

    i->req.inst.raw = ifetch_if.rsp.ir;
    i->req.pc = ifu->pc;
    i->req.is_boot_code = ADDR_IN(ifu->pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);
}

void ifu_update(ifu_t *ifu, const iexec_if_t *i)
{
    if (i->rsp.taken) {
        if (i->rsp.abs_jump) {
            ifu->pc = i->rsp.target_pc;
        } else {
            ifu->pc += i->rsp.pc_offset;
        }
    } else {
        ifu->pc += 4;
    }
}