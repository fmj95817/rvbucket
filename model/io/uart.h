#ifndef UART_H
#define UART_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/apb_req_if.h"
#include "itf/apb_rsp_if.h"
#include "itf/uart_if.h"

typedef struct uart {
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    itf_t *uart_tx_mst;
    itf_t *uart_rx_slv;

    u32 base_addr;
    u32 size;
} uart_t;

extern void uart_construct(uart_t *uart, const char *name, u32 base_addr, u32 size);
extern void uart_reset(uart_t *uart);
extern void uart_clock(uart_t *uart);
extern void uart_free(uart_t *uart);

#endif