#include "biu.h"

void biu_construct(biu_t *biu, bus_if_t *bus_if)
{
    biu->bus_if = bus_if;
}

void biu_reset(biu_t *biu) {}

void biu_free(biu_t *biu) {}

void biu_lsu_trans_handler(biu_t *biu, lsu2biu_trans_t *t)
{
    t->rsp = biu->bus_if->req_handler(biu->bus_if->dev, &t->req);
}

void biu_ifu_trans_handler(biu_t *biu, ifu2biu_trans_t *t)
{
    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = t->req.pc };
    bus_rsp_t rsp = biu->bus_if->req_handler(biu->bus_if->dev, &req);

    t->rsp.ir = rsp.data;
    t->rsp.ok = rsp.ok;
}