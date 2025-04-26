#ifndef UART_H
#define UART_H

#include "base/types.h"
#include "base/itf.h"
#include "itf/bti_req_if.h"
#include "itf/bti_rsp_if.h"

typedef struct uart_if {
    u32 data;
} uart_if_t;

static inline void uart_if_to_str(const void *pkt, char *str)
{
    const uart_if_t *uart_if = (const uart_if_t *)pkt;
    sprintf(str, "%x\n", uart_if->data);
}

typedef struct uart {
    itf_t *bti_req_slv;
    itf_t *bti_rsp_mst;
    itf_t *uart_tx;

    u32 base_addr;
    u32 size;
} uart_t;

extern void uart_construct(uart_t *uart, u32 base_addr, u32 size);
extern void uart_reset(uart_t *uart);
extern void uart_clock(uart_t *uart);
extern void uart_free(uart_t *uart);

#endif