#include "uart.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define REG_BC_ADDR  0u
#define REG_TX_ADDR  1u
#define REG_RX_ADDR  2u
#define REG_STS_ADDR 3u

#define STS_RX_VALID_BIT (1u << 0)

static void uart_update_irq(uart_t *uart)
{
    bool irq = uart->rx_valid;
    if (irq != uart->irq_o->irq) {
        uart->irq_o->irq = irq;
        itf_signal_write_notify(uart->irq_out);
    }
}

static void uart_rx_proc(uart_t *uart)
{
    if (uart->uart_rx_slv == NULL) {
        return;
    }
    if (uart->rx_valid) {
        return;
    }
    if (itf_fifo_empty(uart->uart_rx_slv)) {
        return;
    }

    uart_if_t uart_if;
    itf_read(uart->uart_rx_slv, &uart_if);
    uart->rx_data = uart_if.data;
    uart->rx_valid = true;
}

static void uart_apb_proc(uart_t *uart)
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

    apb_rsp_if_t apb_rsp;
    apb_rsp.pslverr = false;
    apb_rsp.prdata = 0;

    if (apb_req.pwrite) {
        if (reg_addr == REG_TX_ADDR) {
            uart_if_t uart_if;
            uart_if.data = apb_req.pwdata;
            itf_write(uart->uart_tx_mst, &uart_if);
        }
    } else {
        switch (reg_addr) {
        case REG_RX_ADDR:
            apb_rsp.prdata = uart->rx_data;
            uart->rx_valid = false;
            break;
        case REG_STS_ADDR:
            apb_rsp.prdata = uart->rx_valid ? STS_RX_VALID_BIT : 0;
            break;
        }
    }

    itf_write(uart->apb_rsp_mst, &apb_rsp);
}

void uart_construct(uart_t *uart, const char *parent, const char *name, u32 base_addr, u32 size)
{
    mod_construct(&uart->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    uart->base_addr = base_addr;
    uart->size = size;
    uart->irq_o = itf_signal_get_src_and_chk(uart->irq_out);
}

void uart_reset(uart_t *uart)
{
    mod_reset(&uart->mod);
    uart->rx_data = 0;
    uart->rx_valid = false;
    uart->irq_o->irq = false;
    itf_signal_write_notify(uart->irq_out);
}

void uart_clock(uart_t *uart)
{
    mod_clock(&uart->mod);
    uart_rx_proc(uart);
    uart_apb_proc(uart);
    uart_update_irq(uart);
}

void uart_free(uart_t *uart)
{
    mod_free(&uart->mod);
    (void)uart;
}
