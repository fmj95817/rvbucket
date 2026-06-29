#include "timer.h"

#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))

static volatile int timer_done;

void timer_irq_handler(void)
{
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_CTRL, 0u);
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_IRQ, 1u);
    timer_done = 1;
}

void timer_start(uint32_t cycles)
{
    timer_done = 0;
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_IRQ, 1u);
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_RELOAD, cycles);
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_CTRL, TIMER_CTRL_ENABLE);
}

void timer_wait(void)
{
    while (!timer_done) {
        __asm__ volatile ("wfi");
    }
}

uint32_t timer_count(void)
{
    return MMIO_READ32(TIMER_BASE_ADDR + TIMER_REG_COUNT);
}
