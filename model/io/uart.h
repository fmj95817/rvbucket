#ifndef UART_H
#define UART_H

#include "types.h"
#include "bus.h"

typedef struct uart_output {
    u32 ch;
    bool valid;
} uart_output_t;
typedef struct uart {
    u32 ch;
    uart_output_t *output;
} uart_t;

extern void uart_construct(uart_t *uart, uart_output_t *output);
extern void uart_reset(uart_t *uart);
extern void uart_free(uart_t *uart);

extern bool uart_write(uart_t *uart, u32 addr, u32 data, u8 strobe);
extern bus_rsp_t uart_bus_req_handler(uart_t *uart, u32 base_addr, const bus_req_t *req);

#endif