#ifndef UART_H
#define UART_H

#include "base/types.h"
#include "base/itf.h"
#include "bti.h"

typedef struct uart_if {
    u32 rx;
    u32 tx;
} uart_if_t;

typedef struct uart {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;
    itf_t *uart_mst;

    u32 base_addr;
} uart_t;

extern void uart_construct(uart_t *uart, u32 base_addr);
extern void uart_reset(uart_t *uart);
extern void uart_clock(uart_t *uart);
extern void uart_free(uart_t *uart);

#endif