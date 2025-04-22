#include "uart.h"

#define UART_BASE_ADDR 0x30000000
#define UART_BC_REG    0
#define UART_TX_REG    4
#define UART_RX_REG    8

static void uart_write_reg(uint32_t offset, uint32_t val)
{
    volatile uint32_t *reg = (volatile uint32_t *)(UART_BASE_ADDR + offset);
    *reg = val;
}

void uart_init(uint32_t bc)
{
    uart_write_reg(UART_BC_REG, bc);
}

void uart_write(char ch)
{
    uart_write_reg(UART_TX_REG, ch);
}