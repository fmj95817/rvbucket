#ifndef GPIO_IF_H
#define GPIO_IF_H

#include <stdio.h>
#include "base/types.h"
#include "base/def.h"
#include "dbg/vcd.h"

#define GPIO_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) do { \
    itf_conf_t conf = { \
        .cycle = module->mod.cycle, \
        .hier_name = module->mod.hier_name, \
        .mode = ITF_MODE_SIGNAL, \
        .pkt_size = sizeof(gpio_if_t), \
        .pkt2str = &gpio_if_to_str, \
        .reg_vcd = &gpio_if_reg_vcd, \
        .force_disable_trace = dis_trace, \
        .ext_sig_src = ext_src \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct gpio_if {
    u32 val;
} gpio_if_t;

static inline void gpio_if_to_str(const void *pkt, char *str)
{
    const gpio_if_t *gpio = (const gpio_if_t *)pkt;
    sprintf(str, "%08x\n", gpio->val);
}

static inline void gpio_if_reg_vcd(const void *pkt, dbg_sig_type_t type)
{
    const gpio_if_t *gpio = (const gpio_if_t *)pkt;
    dbg_vcd_add_sig("val", type, 32, &gpio->val);
}

#endif
