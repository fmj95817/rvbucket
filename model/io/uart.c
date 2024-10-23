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

void uart_bus_trans_handler(uart_t *uart, u32 base_addr, bus_trans_if_t *i)
{
    DBG_CHECK(i->req.addr >= base_addr);

    u32 addr = i->req.addr - base_addr;
    if (i->req.cmd == BUS_CMD_WRITE) {
        i->rsp.ok = uart_write(uart, addr, i->req.data, i->req.strobe);
    } else {
        i->rsp.ok = false;
    }
}