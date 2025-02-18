#include "uart.h"
#include "dbg/chk.h"

void uart_construct(uart_t *uart, u32 base_addr)
{
    uart->base_addr = base_addr;
}

void uart_reset(uart_t *uart) {}

void uart_free(uart_t *uart) {}

void uart_clock(uart_t *uart)
{
    if (itf_fifo_empty(uart->bti_req_slv)) {
        return;
    }

    if (itf_fifo_full(uart->bti_rsp_mst)) {
        return;
    }

    if (itf_fifo_full(uart->uart_tx)) {
        return;
    }

    bti_req_if_t bti_req;
    itf_read(uart->bti_req_slv, &bti_req);
    DBG_CHECK(bti_req.addr >= uart->base_addr);
    if (bti_req.cmd != BTI_CMD_WRITE) {
        return;
    }

    bti_rsp_if_t bti_rsp;
    bti_rsp.trans_id = bti_req.trans_id;
    bti_rsp.ok = true;
    itf_write(uart->bti_rsp_mst, &bti_rsp);

    uart_if_t uart_if;
    uart_if.data = bti_req.data;
    itf_write(uart->uart_tx, &uart_if);
}