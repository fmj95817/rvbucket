#include "ifu.h"
#include "biu.h"
#include "trans.h"
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

void ifu_exec(ifu_t *ifu, ifu2exu_trans_t *t)
{
    DBG_CHECK(ifu->biu);

    ifu2biu_trans_t mem_read;
    mem_read.req.pc = ifu->pc;

    biu_ifu_trans_handler(ifu->biu, &mem_read);
    DBG_CHECK(mem_read.rsp.ok);

    t->req.inst.raw = mem_read.rsp.ir;
    t->req.pc = ifu->pc;
    t->req.is_boot_code = ADDR_IN(ifu->pc,
        ifu->boot_rom_info.base, ifu->boot_rom_info.size);
}

void ifu_update(ifu_t *ifu, const ifu2exu_trans_t *t)
{
    if (t->rsp.taken) {
        if (t->rsp.abs_jump) {
            ifu->pc = t->rsp.target_pc;
        } else {
            ifu->pc += t->rsp.pc_offset;
        }
    } else {
        ifu->pc += 4;
    }
}