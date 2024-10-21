#include "lsu.h"
#include "dbg.h"

extern void lsu_construct(lsu_t *lsu, biu_t *biu)
{
    lsu->biu = biu;
}

void lsu_reset(lsu_t *lsu) {}

void lsu_free(lsu_t *lsu) {}

void lsu_exu_trans_handler(lsu_t *lsu, exu2lsu_trans_t *t)
{
    DBG_CHECK(lsu->biu);

    lsu2biu_trans_t trans;
    trans.req.cmd = t->req.wr ? BUS_CMD_WRITE : BUS_CMD_READ;
    trans.req.addr = t->req.addr;
    trans.req.data = t->req.data;
    trans.req.strobe = t->req.strobe;

    biu_lsu_trans_handler(lsu->biu, &trans);

    t->rsp.data = trans.rsp.data;
    t->rsp.ok = trans.rsp.ok;
}