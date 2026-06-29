#ifndef SPEC_PERI_H
#define SPEC_PERI_H

#define PERI_UART_OFFSET    0x0000u
#define PERI_UART_SIZE      16u
#define PERI_GPIO_OFFSET    0x1000u
#define PERI_GPIO_SIZE      16u
#define PERI_GTIMER_OFFSET  0x2000u
#define PERI_GTIMER_SIZE    16u

#define GPIO_REG_OUT        0x00u
#define GPIO_REG_MODE_LO    0x04u
#define GPIO_REG_MODE_HI    0x08u

#define GTIMER_REG_CTRL     0x00u
#define GTIMER_REG_COUNT    0x04u
#define GTIMER_REG_RELOAD   0x08u
#define GTIMER_REG_IRQ      0x0Cu

#endif
