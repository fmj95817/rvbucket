#include "gpio.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define REG_OUT_ADDR 0u

void gpio_construct(gpio_t *gpio, const char *name, u32 base_addr, u32 size)
{
    DBG_VCD_MODULE_SCOPE(name);

    gpio->base_addr = base_addr;
    gpio->size = size;
    gpio->out_o = itf_signal_get_src_and_chk(gpio->out_sig);

    dbg_vcd_add_sig("output_val", DBG_SIG_TYPE_REG, 32, &gpio->output_val);
}

void gpio_reset(gpio_t *gpio)
{
    gpio->output_val = 0;
    gpio->out_o->val = 0;
    itf_signal_write_notify(gpio->out_sig);
}

static void gpio_apb_proc(gpio_t *gpio)
{
    if (itf_fifo_empty(gpio->apb_req_slv)) {
        return;
    }
    if (itf_fifo_full(gpio->apb_rsp_mst)) {
        return;
    }

    apb_req_if_t req;
    itf_read(gpio->apb_req_slv, &req);
    DBG_CHECK(req.paddr >= gpio->base_addr);
    DBG_CHECK(req.paddr < gpio->base_addr + gpio->size);

    const u32 reg_addr = (req.paddr - gpio->base_addr) >> 2u;

    apb_rsp_if_t rsp;
    rsp.pslverr = false;
    rsp.prdata = 0;

    if (req.pwrite && reg_addr == REG_OUT_ADDR) {
        gpio->output_val = req.pwdata;
        gpio->out_o->val = req.pwdata;
        itf_signal_write_notify(gpio->out_sig);
    } else if (!req.pwrite && reg_addr == REG_OUT_ADDR) {
        rsp.prdata = gpio->output_val;
    }

    itf_write(gpio->apb_rsp_mst, &rsp);
}

void gpio_clock(gpio_t *gpio)
{
    gpio_apb_proc(gpio);
}

void gpio_free(gpio_t *gpio)
{
    (void)gpio;
}
