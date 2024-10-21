#include "biu.h"

void biu_construct(biu_t *biu, bus_if_t *bus_if)
{
    biu->bus_if = bus_if;
}

void biu_reset(biu_t *biu) {}

void biu_free(biu_t *biu) {}

void biu_mem_rw_trans_handler(biu_t *biu, mem_rw_if_t *i)
{
    i->rsp = biu->bus_if->req_handler(biu->bus_if->dev, &i->req);
}

void biu_ifetch_trans_handler(biu_t *biu, ifetch_if_t *i)
{
    bus_req_t req = { .cmd = BUS_CMD_READ, .addr = i->req.pc };
    bus_rsp_t rsp = biu->bus_if->req_handler(biu->bus_if->dev, &req);

    i->rsp.ir = rsp.data;
    i->rsp.ok = rsp.ok;
}