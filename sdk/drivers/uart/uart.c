#include "uart.h"
#include <stdint.h>

#define UART_BASE_ADDR 0x30000000

void uart_write(char ch)
{
    volatile uint32_t *uart = (volatile uint32_t *)UART_BASE_ADDR;
    *uart = ch;
}