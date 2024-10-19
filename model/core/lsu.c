#include "lsu.h"

void lsu_construct(lsu_t *lsu, bus_if_t *bus_if)
{
    lsu->bus_if = bus_if;
}

void lsu_reset(lsu_t *lsu) {}

void lsu_free(lsu_t *lsu) {}

void lsu_exu_trans_handler(lsu_t *lsu, exu2lsu_trans_t *t)
{
    t->rsp = lsu->bus_if->req_handler(lsu->bus_if->dev, &t->req);
}

void lsu_ifu_trans_handler(lsu_t *lsu, ifu2lsu_trans_t *t)
{
    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = t->req.pc };
    bus_rsp_t rsp = lsu->bus_if->req_handler(lsu->bus_if->dev, &req);

    t->rsp.ir = rsp.data;
    t->rsp.ok = rsp.ok;
}