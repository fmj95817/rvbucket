#ifndef GPIO_H
#define GPIO_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/gpio_if.h"
#include "itf/ext_irq_if.h"

#define GPIO_PIN_NUM    24u
#define GPIO_MODE_OUT   0u
#define GPIO_MODE_IN    1u
#define GPIO_MODE_IN_IRQ 2u

typedef struct gpio {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *inout_sig;          /* bidirectional signal (SoC ↔ sim / UI) */
    itf_t *irq_out;            /* interrupt output */

    u32 output_val;            /* output register */
    u32 mode_lo;               /* mode for pins  0–15 (2 bits each) */
    u32 mode_hi;               /* mode for pins 16–23 (2 bits each) */

    u32 base_addr;
    u32 size;

    gpio_if_t *inout_io;
    ext_irq_if_t *irq_o;
    u32 sig_shadow;            /* previous signal val for edge detection */
} gpio_t;

extern void gpio_construct(gpio_t *gpio, const char *parent, const char *name, u32 base_addr, u32 size);
extern void gpio_reset(gpio_t *gpio);
extern void gpio_clock(gpio_t *gpio);
extern void gpio_free(gpio_t *gpio);

#endif
