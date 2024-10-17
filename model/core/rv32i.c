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
}

void rv32i_free(rv32i_t *s) {}

void rv32i_exec(rv32i_t *s)
{
    DBG_CHECK(s->bus_if);

    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = s->pc };
    bus_rsp_t rsp = s->bus_if->req_handler(s->bus_if->dev, &req);

    rv32i_inst_t i;
    i.raw = rsp.data;

    u32 pc_offset = 4;
    inst_handler(s, &i, &pc_offset);
    s->pc += pc_offset;
}