#include "uart.h"
#include "dbg/chk.h"

#define REG_BC_ADDR 0u
#define REG_TX_ADDR 1u
#define REG_RX_ADDR 2u

void uart_construct(uart_t *uart, u32 base_addr, u32 size)
{
    uart->base_addr = base_addr;
    uart->size = size;
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
    DBG_CHECK(bti_req.addr < uart->base_addr + uart->size);

    const u32 reg_addr = (bti_req.addr - uart->base_addr) >> 2u;
    if (bti_req.cmd == BTI_CMD_WRITE && reg_addr == REG_TX_ADDR) {
        uart_if_t uart_if;
        uart_if.data = bti_req.data;
        itf_write(uart->uart_tx, &uart_if);
    }

    bti_rsp_if_t bti_rsp;
    bti_rsp.trans_id = bti_req.trans_id;
    bti_rsp.ok = true;
    bti_rsp.data = 0;
    itf_write(uart->bti_rsp_mst, &bti_rsp);
}