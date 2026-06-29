#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define GPIO_BASE_ADDR      0x30001000u
#define GPIO_REG_OUT        0x00u
#define GPIO_REG_MODE_LO    0x04u
#define GPIO_REG_MODE_HI    0x08u

#define GPIO_MODE_OUT       0u
#define GPIO_MODE_IN        1u
#define GPIO_MODE_IN_IRQ    2u

extern void gpio_write(uint32_t val);
extern uint32_t gpio_read(void);
extern void gpio_set_mode(uint32_t pin, uint32_t mode);
extern void gpio_set_irq_callback(void (*cb)(void));
extern void gpio_irq_handler(void);

#endif
