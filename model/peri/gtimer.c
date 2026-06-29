#include "gtimer.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define REG_CTRL    0u
#define REG_COUNT   1u
#define REG_RELOAD  2u
#define REG_IRQ     3u

void gtimer_construct(gtimer_t *t, const char *name, u32 base, u32 size)
{
    DBG_VCD_MODULE_SCOPE(name);
    t->base_addr = base;
    t->size = size;
    t->irq_o = itf_signal_get_src_and_chk(t->irq_out);
    dbg_vcd_add_sig("count", DBG_SIG_TYPE_REG, 32, &t->count);
}

void gtimer_reset(gtimer_t *t)
{
    t->ctrl = 0;
    t->count = 0;
    t->reload = 0;
    t->irq_pend = false;
    t->irq_o->irq = false;
    itf_signal_write_notify(t->irq_out);
}

static void gtimer_apb_proc(gtimer_t *t)
{
    if (itf_fifo_empty(t->apb_req_slv)) {
        return;
    }
    if (itf_fifo_full(t->apb_rsp_mst)){
        return;
    }

    apb_req_if_t req;
    itf_read(t->apb_req_slv, &req);
    DBG_CHECK(req.paddr >= t->base_addr);
    DBG_CHECK(req.paddr < t->base_addr + t->size);

    u32 reg = (req.paddr - t->base_addr) >> 2u;
    apb_rsp_if_t rsp = { .pslverr = false, .prdata = 0 };

    if (req.pwrite) {
        switch (reg) {
        case REG_CTRL:
            t->ctrl = req.pwdata & 1u;
            break;
        case REG_RELOAD:
            t->reload = req.pwdata;
            t->count = req.pwdata;
            t->irq_pend = false;
            t->irq_o->irq = false;
            itf_signal_write_notify(t->irq_out);
            break;
        case REG_IRQ:
            if (req.pwdata & 1u) {
                t->irq_pend = false;
                t->irq_o->irq = false;
                itf_signal_write_notify(t->irq_out);
            }
            break;
        }
    } else {
        switch (reg) {
        case REG_CTRL:
            rsp.prdata = t->ctrl;
            break;
        case REG_COUNT:
            rsp.prdata = t->count;
            break;
        case REG_RELOAD:
            rsp.prdata = t->reload;
            break;
        case REG_IRQ:
            rsp.prdata = t->irq_pend ? 1u : 0u;
            break;
        }
    }
    itf_write(t->apb_rsp_mst, &rsp);
}

static void gtimer_count_proc(gtimer_t *t)
{
    if (!(t->ctrl & 1u)) {
        return;
    }
    if (t->count == 0) {
        return;
    }

    t->count--;
    if (t->count == 0) {
        t->irq_pend = true;
        t->irq_o->irq = true;
        itf_signal_write_notify(t->irq_out);
    }
}

void gtimer_clock(gtimer_t *t)
{
    gtimer_count_proc(t);
    gtimer_apb_proc(t);
}

void gtimer_free(gtimer_t *t)
{
    (void)t;
}
