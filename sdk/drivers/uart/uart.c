#include "uart.h"

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
    return (char)uart_read_reg(UART_REG_RX);
}

uint32_t uart_status(void)
{
    return uart_read_reg(UART_REG_STS);
}
