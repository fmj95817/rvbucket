#include "rv32i.h"
#include "isa.h"
#include "exu.h"
#include "dbg.h"

void rv32i_construct(rv32i_t *s, bus_if_t *bus_if, u32 reset_pc)
{
    s->bus_if = bus_if;
    s->reset_pc = reset_pc;
}

void rv32i_reset(rv32i_t *s)
{
    s->pc = s->reset_pc;
    s->gpr[0] = 0;
    for (int i = 1; i < 32; i++) {
        s->gpr[i] = (u32)rand();
    }
}

void rv32i_free(rv32i_t *s) {}

static void rv32i_dump(rv32i_t *s)
{
    DBG_PRINT("pc = %08x\n", s->pc);
    DBG_PRINT("x0/zero  = %08x     x1/ra = %08x     x2/sp  = %08x     x3/gp  = %08x\n", s->gpr[0], s->gpr[1], s->gpr[2], s->gpr[3]);
    DBG_PRINT("x4/tp    = %08x     x5/t0 = %08x     x6/t1  = %08x     x7/t2  = %08x\n", s->gpr[4], s->gpr[5], s->gpr[6], s->gpr[7]);
    DBG_PRINT("x8/s0/fp = %08x     x9/s1 = %08x    x10/a0  = %08x    x11/a1  = %08x\n", s->gpr[8], s->gpr[9], s->gpr[10], s->gpr[11]);
    DBG_PRINT("x12/a2   = %08x    x13/a3 = %08x    x14/a4  = %08x    x15/a5  = %08x\n", s->gpr[12], s->gpr[13], s->gpr[14], s->gpr[15]);
    DBG_PRINT("x16/a6   = %08x    x17/a7 = %08x    x18/s2  = %08x    x19/s3  = %08x\n", s->gpr[16], s->gpr[17], s->gpr[18], s->gpr[19]);
    DBG_PRINT("x20/s4   = %08x    x21/s5 = %08x    x22/s6  = %08x    x23/s7  = %08x\n", s->gpr[20], s->gpr[21], s->gpr[22], s->gpr[23]);
    DBG_PRINT("x24/s8   = %08x    x25/s9 = %08x    x26/s10 = %08x    x27/s11 = %08x\n", s->gpr[24], s->gpr[25], s->gpr[26], s->gpr[27]);
    DBG_PRINT("x28/t3   = %08x    x29/t4 = %08x    x30/t5  = %08x    x31/t6  = %08x\n\n", s->gpr[28], s->gpr[29], s->gpr[30], s->gpr[31]);
}

void rv32i_exec(rv32i_t *s)
{
    DBG_CHECK(s->bus_if);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = s->pc };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);

    rv32i_inst_t i;
    i.raw = rsp.data;

    // rv32i_dump(s);

    u32 pc_offset = 4;
    inst_handler(s, &i, &pc_offset);
    s->pc += pc_offset;
}