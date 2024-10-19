#include "uart.h"
#include "dbg.h"

void uart_construct(uart_t *uart, uart_output_t *output)
{
    uart->output = output;
}

void uart_reset(uart_t *uart)
{
    uart->output->valid = false;
}

void uart_free(uart_t *uart) {}

static void uart_output(uart_t *uart)
{
    uart->output->ch = uart->ch;
    uart->output->valid = true;
}

bool uart_write(uart_t *uart, u32 addr, u32 data, u8 strobe)
{
    if (addr > 0) {
        return false;
    }

    uart->ch = data;
    uart_output(uart);

    return true;
}

bus_rsp_t uart_bus_req_handler(uart_t *uart, u32 base_addr, const bus_req_t *req)
{
    DBG_CHECK(req->addr >= base_addr);

    u32 addr = req->addr - base_addr;
    bus_rsp_t rsp;

    if (req->cmd == BUS_CMD_WRITE) {
        rsp.ok = uart_write(uart, addr, req->data, req->strobe);
    } else {
        rsp.ok = false;
    }

    return rsp;
}