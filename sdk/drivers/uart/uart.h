#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_BASE_ADDR  0x30000000u
#define UART_REG_BC     0x00u
#define UART_REG_TX     0x04u
#define UART_REG_RX     0x08u
#define UART_REG_STS    0x0Cu

#define UART_STS_RX_VALID   (1u << 0)

#define UART_IRQ_NUM    1

extern void uart_init(uint32_t bc);
extern void uart_write(char ch);
extern char uart_read(void);
extern uint32_t uart_status(void);

#endif
