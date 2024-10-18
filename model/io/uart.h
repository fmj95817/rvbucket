#ifndef UART_H
#define UART_H

#include "types.h"
#include "bus.h"

typedef struct uart {
    u32 ch;
} uart_t;

extern void uart_construct(uart_t *uart);
extern void uart_reset(uart_t *uart);
extern void uart_free(uart_t *uart);

extern bool uart_write(uart_t *uart, u32 addr, u32 data, u8 strobe);
extern bus_rsp_t uart_bus_req_handler(uart_t *uart, u32 base_addr, const bus_req_t *req);
extern bool uart_end_sim(uart_t *uart);

#endif