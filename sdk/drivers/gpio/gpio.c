#include "gpio.h"

#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))

void gpio_write(uint32_t val)
{
    MMIO_WRITE32(GPIO_BASE_ADDR + GPIO_REG_OUT, val);
}

uint32_t gpio_read(void)
{
    return MMIO_READ32(GPIO_BASE_ADDR + GPIO_REG_OUT);
}
