#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define GPIO_BASE_ADDR  0x30001000u
#define GPIO_REG_OUT    0x00u

extern void gpio_write(uint32_t val);
extern uint32_t gpio_read(void);

#endif
