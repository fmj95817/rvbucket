#include "uart.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define REG_BC_ADDR 0u
#define REG_TX_ADDR 1u
#define REG_RX_ADDR 2u

void uart_construct(uart_t *uart, const char *name, u32 base_addr, u32 size)
{
    DBG_VCD_MODULE_SCOPE(name);

    uart->base_addr = base_addr;
    uart->size = size;
}

void uart_reset(uart_t *uart) {}

void uart_free(uart_t *uart) {}

void uart_clock(uart_t *uart)
{
    if (itf_fifo_empty(uart->apb_req_slv)) {
        return;
    }

    if (itf_fifo_full(uart->apb_rsp_mst)) {
        return;
    }

    if (itf_fifo_full(uart->uart_tx_mst)) {
        return;
    }

    apb_req_if_t apb_req;
    itf_read(uart->apb_req_slv, &apb_req);
    DBG_CHECK(apb_req.paddr >= uart->base_addr);
    DBG_CHECK(apb_req.paddr < uart->base_addr + uart->size);

    const u32 reg_addr = (apb_req.paddr - uart->base_addr) >> 2u;
    if (apb_req.pwrite && reg_addr == REG_TX_ADDR) {
        uart_if_t uart_if;
        uart_if.data = apb_req.pwdata;
        itf_write(uart->uart_tx_mst, &uart_if);
    }

    apb_rsp_if_t apb_rsp;
    apb_rsp.pslverr = false;
    apb_rsp.prdata = 0;
    itf_write(uart->apb_rsp_mst, &apb_rsp);
}