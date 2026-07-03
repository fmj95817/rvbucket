#ifndef UART_H
#define UART_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/uart_if.h"
#include "itf/ext_irq_if.h"

typedef struct uart {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;
    itf_t *irq_out;

    u32 base_addr;
    u32 size;

    ext_irq_if_t *irq_o;
    u32 rx_data;
    bool rx_valid;
} uart_t;

extern void uart_construct(uart_t *uart, const char *parent, const char *name, u32 base_addr, u32 size);
extern void uart_reset(uart_t *uart);
extern void uart_clock(uart_t *uart);
extern void uart_free(uart_t *uart);

#endif
