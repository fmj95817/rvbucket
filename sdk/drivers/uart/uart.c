#include "uart.h"

#define UART_RX_BUF_SIZE 256

static char rx_buf[UART_RX_BUF_SIZE];
static volatile int rx_rd, rx_wr;

static void uart_write_reg(uint32_t offset, uint32_t val)
{
    volatile uint32_t *reg = (volatile uint32_t *)(UART_BASE_ADDR + offset);
    *reg = val;
}

static uint32_t uart_read_reg(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(UART_BASE_ADDR + offset);
    return *reg;
}

void uart_init(uint32_t bc)
{
    uart_write_reg(UART_REG_BC, bc);
}

void uart_write(char ch)
{
    uart_write_reg(UART_REG_TX, (uint32_t)ch);
}

char uart_read(void)
{
    if (rx_rd == rx_wr) {
        return 0;
    }
    char ch = rx_buf[rx_rd];
    rx_rd = (rx_rd + 1) % UART_RX_BUF_SIZE;
    return ch;
}

uint32_t uart_status(void)
{
    return uart_read_reg(UART_REG_STS);
}

void uart_irq_handler(void)
{
    while (uart_status() & UART_STS_RX_VALID) {
        char ch = (char)uart_read_reg(UART_REG_RX);

        if (ch == 0x7f || ch == '\b') {
            /* echo backspace escape sequence to erase on screen */
            uart_write('\b');
            uart_write(' ');
            uart_write('\b');
        } else {
            /* echo */
            uart_write(ch);
            if (ch == '\r') {
                uart_write('\n');
            }
        }

        int nw = (rx_wr + 1) % UART_RX_BUF_SIZE;
        if (nw != rx_rd) {
            rx_buf[rx_wr] = ch;
            rx_wr = nw;
        }
    }
}
