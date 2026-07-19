#include <stdint.h>
#include <stdio.h>

#define ACLINT_MTIME_ADDR       0x31000000u
#define ACLINT_MTIMECMP_ADDR    0x31010000u
#define MCAUSE_INTERRUPT_BIT    0x80000000u
#define MCAUSE_M_TIMER          7u
#define IRQ_TARGET               32u
#define LOOP_COUNT              200000u

extern const char branch_loop_begin[];
extern const char branch_loop_end[];

static volatile uint32_t irq_count;
static volatile uint32_t bad_cause;
static volatile uint32_t bad_mepc;

static volatile uint32_t *const mtime_lo =
    (volatile uint32_t *)ACLINT_MTIME_ADDR;
static volatile uint32_t *const mtime_hi =
    (volatile uint32_t *)(ACLINT_MTIME_ADDR + 4u);
static volatile uint32_t *const mtimecmp_lo =
    (volatile uint32_t *)ACLINT_MTIMECMP_ADDR;
static volatile uint32_t *const mtimecmp_hi =
    (volatile uint32_t *)(ACLINT_MTIMECMP_ADDR + 4u);

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
    *mtimecmp_hi = UINT32_MAX;
    *mtimecmp_lo = (uint32_t)val;
    *mtimecmp_hi = (uint32_t)(val >> 32u);
}

uint32_t trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mtval;

    if (mcause != (MCAUSE_INTERRUPT_BIT | MCAUSE_M_TIMER)) {
        bad_cause = mcause;
        mtimecmp_write(UINT64_MAX);
        return mepc;
    }

    if (mepc < (uint32_t)(uintptr_t)branch_loop_begin ||
        mepc >= (uint32_t)(uintptr_t)branch_loop_end) {
        bad_mepc = mepc;
    }

    irq_count++;
    if (irq_count < IRQ_TARGET)
        mtimecmp_write(mtime_read() + 1u);
    else
        mtimecmp_write(UINT64_MAX);

    return mepc;
}

static uint32_t run_branch_loop(void)
{
    uint32_t count;

    asm volatile(
        "li t3, 0\n"
        "li t4, %1\n"
        ".global branch_loop_begin\n"
        "branch_loop_begin:\n"
        "addi t3, t3, 1\n"
        "andi t5, t3, 7\n"
        "beqz t5, 1f\n"
        "addi t6, t6, 0\n"
        "1:\n"
        "bne t3, t4, branch_loop_begin\n"
        ".global branch_loop_end\n"
        "branch_loop_end:\n"
        "mv %0, t3\n"
        : "=r"(count)
        : "i"(LOOP_COUNT)
        : "t3", "t4", "t5", "t6", "memory");

    return count;
}

int main(void)
{
    irq_count = 0;
    bad_cause = 0;
    bad_mepc = 0;

    mtimecmp_write(UINT64_MAX);
    mtimecmp_write(mtime_read() + 1u);
    uint32_t count = run_branch_loop();
    mtimecmp_write(UINT64_MAX);

    if (count == LOOP_COUNT && irq_count == IRQ_TARGET &&
        bad_cause == 0u && bad_mepc == 0u) {
        printf("irq_branch_stress: PASS irq=%u count=%u\n",
            (unsigned int)irq_count, (unsigned int)count);
        return 0;
    }

    printf("irq_branch_stress: FAIL irq=%u count=%u cause=0x%08x "
        "mepc=0x%08x\n", (unsigned int)irq_count, (unsigned int)count,
        (unsigned int)bad_cause, (unsigned int)bad_mepc);
    return 1;
}
