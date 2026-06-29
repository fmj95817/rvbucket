#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define TIMER_BASE_ADDR     0x30002000u
#define TIMER_REG_CTRL      0x00u
#define TIMER_REG_COUNT     0x04u
#define TIMER_REG_RELOAD    0x08u
#define TIMER_REG_IRQ       0x0Cu

#define TIMER_CTRL_ENABLE   1u

extern void timer_start(uint32_t cycles);
extern void timer_wait(void);
extern uint32_t timer_count(void);

#endif
