#ifndef UART_H
#define UART_H

#include <stdint.h>

extern void uart_init(uint32_t bc);
extern void uart_write(char ch);

#endif