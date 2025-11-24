#ifndef UART_IF_H
#define UART_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define UART_IF_CONSTRUCT(module, itf, depth) do { \
    itf_conf_t conf = { \
        .cycle = module->cycle, \
        .pkt2str = &uart_if_to_str, \
        .reg_vcd = &uart_if_reg_vcd, \
        .pkt_size = sizeof(uart_if_t), \
        .fifo_depth = depth \
    }; \
    itf_construct(&module->itf, #itf, &conf); \
} while (0)

typedef struct uart_if {
    u32 data;
} uart_if_t;

static inline void uart_if_to_str(const void *pkt, char *str)
{
    const uart_if_t *uart = (const uart_if_t *)pkt;
    sprintf(str, "%08x\n", uart->data);
}

static inline void uart_if_reg_vcd(const void *pkt)
{
    const uart_if_t *uart = (const uart_if_t *)pkt;
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &uart->data);
}

#endif
