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

void gpio_set_mode(uint32_t pin, uint32_t mode)
{
    uint32_t reg_off = (pin < 16u) ? GPIO_REG_MODE_LO : GPIO_REG_MODE_HI;
    uint32_t shift = (pin & 0xfu) * 2u;
    uint32_t addr = GPIO_BASE_ADDR + reg_off;
    uint32_t cur = MMIO_READ32(addr);
    cur &= ~(0x3u << shift);
    cur |= (mode & 0x3u) << shift;
    MMIO_WRITE32(addr, cur);
}
