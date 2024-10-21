#include "lsu.h"
#include "dbg.h"

extern void lsu_construct(lsu_t *lsu, biu_t *biu)
{
    lsu->biu = biu;
}

void lsu_reset(lsu_t *lsu) {}

void lsu_free(lsu_t *lsu) {}

void lsu_ldst_trans_handler(lsu_t *lsu, ldst_if_t *i)
{
    DBG_CHECK(lsu->biu);

    mem_rw_if_t mem_rw_if;
    mem_rw_if.req.cmd = i->req.wr ? BUS_CMD_WRITE : BUS_CMD_READ;
    mem_rw_if.req.addr = i->req.addr;
    mem_rw_if.req.data = i->req.data;
    mem_rw_if.req.strobe = i->req.strobe;

    biu_mem_rw_trans_handler(lsu->biu, &mem_rw_if);

    i->rsp.data = mem_rw_if.rsp.data;
    i->rsp.ok = mem_rw_if.rsp.ok;
}