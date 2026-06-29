#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define GTIMER_BASE_ADDR     0x30002000u
#define GTIMER_REG_CTRL      0x00u
#define GTIMER_REG_COUNT     0x04u
#define GTIMER_REG_RELOAD    0x08u
#define GTIMER_REG_IRQ       0x0Cu

#define GTIMER_CTRL_ENABLE   1u

extern void gtimer_start(uint32_t cycles);
extern void gtimer_wait(void);
extern uint32_t gtimer_count(void);

#endif
