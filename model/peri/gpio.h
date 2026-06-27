#ifndef GPIO_H
#define GPIO_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/gpio_if.h"

typedef struct gpio {
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *out_sig;

    u32 base_addr;
    u32 size;

    gpio_if_t *out_o;
    u32 output_val;
} gpio_t;

extern void gpio_construct(gpio_t *gpio, const char *name, u32 base_addr, u32 size);
extern void gpio_reset(gpio_t *gpio);
extern void gpio_clock(gpio_t *gpio);
extern void gpio_free(gpio_t *gpio);

#endif
