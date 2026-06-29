#include "gpio.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define REG_OUT       0u
#define REG_MODE_LO   1u
#define REG_MODE_HI   2u

static inline u32 gpio_pin_mode(const gpio_t *gpio, u32 pin)
{
    u32 reg = (pin < 16u) ? gpio->mode_lo : gpio->mode_hi;
    u32 shift = (pin & 0xfu) * 2u;
    return (reg >> shift) & 0x3u;
}

void gpio_construct(gpio_t *gpio, const char *name, u32 base_addr, u32 size)
{
    DBG_VCD_MODULE_SCOPE(name);

    gpio->base_addr = base_addr;
    gpio->size = size;
    gpio->inout_io = itf_signal_get_src_and_chk(gpio->inout_sig);
    gpio->irq_o = itf_signal_get_src_and_chk(gpio->irq_out);

    dbg_vcd_add_sig("output_val", DBG_SIG_TYPE_REG, 32, &gpio->output_val);
    dbg_vcd_add_sig("mode_lo", DBG_SIG_TYPE_REG, 32, &gpio->mode_lo);
    dbg_vcd_add_sig("mode_hi", DBG_SIG_TYPE_REG, 32, &gpio->mode_hi);
}

void gpio_reset(gpio_t *gpio)
{
    gpio->output_val = 0;
    gpio->mode_lo = 0;
    gpio->mode_hi = 0;
    gpio->inout_io->val = 0;
    gpio->sig_shadow = 0;
    gpio->irq_o->irq = false;
    itf_signal_write_notify(gpio->inout_sig);
    itf_signal_write_notify(gpio->irq_out);
}

static void gpio_apb_proc(gpio_t *gpio)
{
    if (itf_fifo_empty(gpio->apb_req_slv)) return;
    if (itf_fifo_full(gpio->apb_rsp_mst)) return;

    apb_req_if_t req;
    itf_read(gpio->apb_req_slv, &req);
    DBG_CHECK(req.paddr >= gpio->base_addr);
    DBG_CHECK(req.paddr < gpio->base_addr + gpio->size);

    const u32 reg_addr = (req.paddr - gpio->base_addr) >> 2u;
    apb_rsp_if_t rsp = { .pslverr = false, .prdata = 0 };

    if (req.pwrite) {
        switch (reg_addr) {
        case REG_OUT:
            gpio->output_val = req.pwdata;
            /* only touch output-mode bits; input bits are preserved
               (they are written by sim_top / UI on the same signal) */
            for (u32 i = 0; i < GPIO_PIN_NUM; i++) {
                if (gpio_pin_mode(gpio, i) != GPIO_MODE_OUT) continue;
                if (req.pwdata & (1u << i))
                    gpio->inout_io->val |= (1u << i);
                else
                    gpio->inout_io->val &= ~(1u << i);
            }
            itf_signal_write_notify(gpio->inout_sig);
            break;
        case REG_MODE_LO:
            gpio->mode_lo = req.pwdata;
            break;
        case REG_MODE_HI:
            gpio->mode_hi = req.pwdata & 0xffffu;
            break;
        }
    } else {
        switch (reg_addr) {
        case REG_OUT: {
            u32 val = gpio->output_val;
            u32 sig = gpio->inout_io->val;
            for (u32 i = 0; i < GPIO_PIN_NUM; i++) {
                u32 m = gpio_pin_mode(gpio, i);
                if (m == GPIO_MODE_IN || m == GPIO_MODE_IN_IRQ) {
                    if (sig & (1u << i)) {
                        val |= (1u << i);
                    } else {
                        val &= ~(1u << i);
                    }
                }
            }
            rsp.prdata = val;
            break;
        }
        case REG_MODE_LO:
            rsp.prdata = gpio->mode_lo;
            break;
        case REG_MODE_HI:
            rsp.prdata = gpio->mode_hi;
            break;
        }
    }

    itf_write(gpio->apb_rsp_mst, &rsp);
}

static void gpio_irq_proc(gpio_t *gpio)
{
    u32 cur = gpio->inout_io->val;
    u32 prev = gpio->sig_shadow;
    gpio->sig_shadow = cur;

    bool irq = false;
    for (u32 i = 0; i < GPIO_PIN_NUM; i++) {
        if (gpio_pin_mode(gpio, i) != GPIO_MODE_IN_IRQ) {
            continue;
        }
        if (((cur >> i) & 1u) != ((prev >> i) & 1u)) {
            irq = true;
        }
    }
    gpio->irq_o->irq = irq;
    if (irq) {
        itf_signal_write_notify(gpio->irq_out);
    }
}

void gpio_clock(gpio_t *gpio)
{
    gpio_apb_proc(gpio);
    gpio_irq_proc(gpio);
}

void gpio_free(gpio_t *gpio)
{
    (void)gpio;
}
