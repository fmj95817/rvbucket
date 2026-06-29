#include "timer.h"

#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))

#define PLIC_BASE          0x31100000u
#define PLIC_CTX_BASE      0x200000u
#define PLIC_CTX_CLAIM     0x004u
#define TIMER_IRQ_SRC      3u

void timer_start(uint32_t cycles)
{
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_IRQ, 1u);
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_RELOAD, cycles);
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_CTRL, TIMER_CTRL_ENABLE);
}

void timer_wait(void)
{
    __asm__ volatile ("wfi");

    uint32_t claim = MMIO_READ32(PLIC_BASE + PLIC_CTX_BASE + PLIC_CTX_CLAIM);
    if (claim) MMIO_WRITE32(PLIC_BASE + PLIC_CTX_BASE + PLIC_CTX_CLAIM, claim);

    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_CTRL, 0u);
    MMIO_WRITE32(TIMER_BASE_ADDR + TIMER_REG_IRQ, 1u);
}

uint32_t timer_count(void)
{
    return MMIO_READ32(TIMER_BASE_ADDR + TIMER_REG_COUNT);
}
