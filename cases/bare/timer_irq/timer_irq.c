#include <stdint.h>
#include <stdio.h>

#define ACLINT_MTIME_ADDR       0x31000000u
#define ACLINT_MTIMECMP_ADDR    0x31010000u

#define MCAUSE_INTERRUPT_BIT    0x80000000u
#define MCAUSE_M_TIMER          7u
static volatile uint32_t timer_irq_seen;
static volatile uint32_t timer_irq_bad_cause;
static volatile uint32_t timer_irq_mepc;

extern const char irq_precision_begin[];
extern const char irq_precision_end[];

static volatile uint32_t *const mtime_lo = (volatile uint32_t *)ACLINT_MTIME_ADDR;
static volatile uint32_t *const mtime_hi = (volatile uint32_t *)(ACLINT_MTIME_ADDR + 4u);
static volatile uint32_t *const mtimecmp_lo = (volatile uint32_t *)ACLINT_MTIMECMP_ADDR;
static volatile uint32_t *const mtimecmp_hi = (volatile uint32_t *)(ACLINT_MTIMECMP_ADDR + 4u);

static uint64_t mtime_read(void)
{
    uint32_t hi0;
    uint32_t lo;
    uint32_t hi1;

    do {
        hi0 = *mtime_hi;
        lo = *mtime_lo;
        hi1 = *mtime_hi;
    } while (hi0 != hi1);

    return ((uint64_t)hi0 << 32) | lo;
}

static void mtimecmp_write(uint64_t val)
{
    *mtimecmp_hi = 0xffffffffu;
    *mtimecmp_lo = (uint32_t)val;
    *mtimecmp_hi = (uint32_t)(val >> 32);
}

uint32_t trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mtval;

    if (mcause == (MCAUSE_INTERRUPT_BIT | MCAUSE_M_TIMER)) {
        timer_irq_seen++;
        timer_irq_mepc = mepc;
        mtimecmp_write(UINT64_MAX);
    } else {
        timer_irq_bad_cause = mcause;
    }

    return mepc;
}

static uint32_t run_irq_precision_sequence(void)
{
    uint32_t count;

    __asm__ volatile (
        "li t3, 0\n"
        "li t4, 65536\n"
        ".global irq_precision_begin\n"
        "irq_precision_begin:\n"
        "addi t3, t3, 1\n"
        "bne t3, t4, irq_precision_begin\n"
        ".global irq_precision_end\n"
        "irq_precision_end:\n"
        "mv %0, t3\n"
        : "=r" (count)
        :
        : "t3", "t4", "memory");

    return count;
}

int main(void)
{
    uint32_t precision_count;

    timer_irq_seen = 0;
    timer_irq_bad_cause = 0;
    timer_irq_mepc = 0;

    mtimecmp_write(UINT64_MAX);
    mtimecmp_write(mtime_read() + 2u);

    for (uint32_t i = 0; i < 4u && timer_irq_seen == 0u && timer_irq_bad_cause == 0u; i++) {
        __asm__ volatile ("wfi");
    }

    if (timer_irq_seen != 1u || timer_irq_bad_cause != 0u) {
        mtimecmp_write(UINT64_MAX);
        printf("timer_irq: FAIL seen=%u bad_cause=0x%08x\n",
            (unsigned int)timer_irq_seen, (unsigned int)timer_irq_bad_cause);
        return 1;
    }

    timer_irq_seen = 0;
    timer_irq_mepc = 0;
    mtimecmp_write(mtime_read() + 1u);
    precision_count = run_irq_precision_sequence();
    mtimecmp_write(UINT64_MAX);

    if (timer_irq_seen == 1u && timer_irq_bad_cause == 0u &&
        precision_count == 65536u &&
        timer_irq_mepc >= (uint32_t)(uintptr_t)irq_precision_begin &&
        timer_irq_mepc < (uint32_t)(uintptr_t)irq_precision_end) {
        printf("timer_irq: PASS\n");
        return 0;
    }

    printf("timer_irq: FAIL seen=%u bad_cause=0x%08x count=%u mepc=0x%08x\n",
        (unsigned int)timer_irq_seen, (unsigned int)timer_irq_bad_cause,
        (unsigned int)precision_count, (unsigned int)timer_irq_mepc);
    return 1;
}
