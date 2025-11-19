#ifndef UART_IF_H
#define UART_IF_H

#include <stdio.h>
#include "base/types.h"
#include "dbg/vcd.h"

#define UART_IF_CONSTRUCT(m, name, depth) itf_construct(&m->name, m->cycle, #name, &uart_if_to_str, &uart_if_reg_vcd_sig, sizeof(uart_if_t), depth)

typedef struct uart_if {
    u32 data;
} uart_if_t;

static inline void uart_if_to_str(const void *pkt, char *str)
{
    const uart_if_t *uart = (const uart_if_t *)pkt;
    sprintf(str, "%08x\n", uart->data);
}

static inline void uart_if_reg_vcd_sig(const void *pkt)
{
    const uart_if_t *uart = (const uart_if_t *)pkt;
    dbg_vcd_add_sig("data", DBG_SIG_TYPE_REG, 32, &uart->data);
}

#endif
