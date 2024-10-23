#include "biu.h"
#include "dbg.h"

void biu_construct(biu_t *biu, bus_if_t *bus_if)
{
    biu->bus_if = bus_if;
}

void biu_reset(biu_t *biu) {}

void biu_free(biu_t *biu) {}

void biu_ldst_trans_handler(biu_t *biu, ldst_if_t *i)
{
    DBG_CHECK(biu->bus_if);

    bus_trans_if_t bus_trans_if;
    bus_trans_if.req.cmd = i->req.st ? BUS_CMD_WRITE : BUS_CMD_READ;
    bus_trans_if.req.addr = i->req.addr;
    bus_trans_if.req.data = i->req.data;
    bus_trans_if.req.strobe = i->req.strobe;
    biu->bus_if->trans_handler(biu->bus_if->dev, &bus_trans_if);

    i->rsp.data = bus_trans_if.rsp.data;
    i->rsp.ok = bus_trans_if.rsp.ok;
}

void biu_ifetch_trans_handler(biu_t *biu, ifetch_if_t *i)
{
    DBG_CHECK(biu->bus_if);

    bus_trans_if_t bus_trans_if;
    bus_trans_if.req.cmd = BUS_CMD_READ;
    bus_trans_if.req.addr = i->req.pc;
    biu->bus_if->trans_handler(biu->bus_if->dev, &bus_trans_if);

    i->rsp.ir = bus_trans_if.rsp.data;
    i->rsp.ok = bus_trans_if.rsp.ok;
}