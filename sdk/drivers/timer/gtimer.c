#include "gtimer.h"

#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))

static volatile int gtimer_done;

void gtimer_irq_handler(void)
{
    MMIO_WRITE32(GTIMER_BASE_ADDR + GTIMER_REG_CTRL, 0u);
    MMIO_WRITE32(GTIMER_BASE_ADDR + GTIMER_REG_IRQ, 1u);
    gtimer_done = 1;
}

void gtimer_start(uint32_t cycles)
{
    gtimer_done = 0;
    MMIO_WRITE32(GTIMER_BASE_ADDR + GTIMER_REG_IRQ, 1u);
    MMIO_WRITE32(GTIMER_BASE_ADDR + GTIMER_REG_RELOAD, cycles);
    MMIO_WRITE32(GTIMER_BASE_ADDR + GTIMER_REG_CTRL, GTIMER_CTRL_ENABLE);
}

void gtimer_wait(void)
{
    while (!gtimer_done) {
        __asm__ volatile ("wfi");
    }
}

uint32_t gtimer_count(void)
{
    return MMIO_READ32(GTIMER_BASE_ADDR + GTIMER_REG_COUNT);
}
