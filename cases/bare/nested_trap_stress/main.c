#include <stdint.h>
#include <stdio.h>

#define ACLINT_MTIME_ADDR       0x31000000u
#define ACLINT_MTIMECMP_ADDR    0x31010000u

#define MSTATUS_MPP_MASK        (3u << 11)
#define MSTATUS_MPP_M           (3u << 11)
#define MSTATUS_MPP_S           (1u << 11)
#define MSTATUS_SIE             (1u << 1)
#define MSTATUS_SPP             (1u << 8)
#define MIE_MTIE                (1u << 7)
#define MIE_STIE                (1u << 5)
#define MIDELEG_S_TIMER         (1u << 5)
#define MCAUSE_INTERRUPT        0x80000000u
#define MCAUSE_M_TIMER          (MCAUSE_INTERRUPT | 7u)
#define MCAUSE_ECALL_S          9u
#define SCAUSE_S_TIMER          (MCAUSE_INTERRUPT | 5u)

#define S_IRQ_TARGET             1u
#define S_TIMER_INTERVAL        64u
#define NESTED_WAIT_LIMIT       20000u

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void m_mode_resume(void);

volatile uint32_t s_irq_count;
volatile uint32_t m_irq_count;
volatile uint32_t bad_mcause;
volatile uint32_t bad_scause;
volatile uint32_t bad_sstatus;
volatile uint32_t nested_timeout;
volatile uint32_t workload_signature;
static uintptr_t m_resume_pc;
volatile uintptr_t m_resume_ra;

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

static uint64_t time_read(void)
{
    uint32_t hi0;
    uint32_t lo;
    uint32_t hi1;

    do {
        asm volatile("rdtimeh %0" : "=r"(hi0));
        asm volatile("rdtime %0" : "=r"(lo));
        asm volatile("rdtimeh %0" : "=r"(hi1));
    } while (hi0 != hi1);

    return ((uint64_t)hi0 << 32) | lo;
}

static void mtimecmp_write(uint64_t val)
{
    *mtimecmp_hi = UINT32_MAX;
    *mtimecmp_lo = (uint32_t)val;
    *mtimecmp_hi = (uint32_t)(val >> 32u);
}

static void stimecmp_write(uint64_t val)
{
    asm volatile("csrw 0x15d, %0" :: "r"((uint32_t)(val >> 32u)) :
        "memory");
    asm volatile("csrw 0x14d, %0" :: "r"((uint32_t)val) : "memory");
}

static uint32_t csr_read_sstatus(void)
{
    uint32_t val;
    asm volatile("csrr %0, sstatus" : "=r"(val));
    return val;
}

static uint32_t csr_read_mstatus(void)
{
    uint32_t val;
    asm volatile("csrr %0, mstatus" : "=r"(val));
    return val;
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc)
{
    if (scause != SCAUSE_S_TIMER) {
        bad_scause = scause;
        stimecmp_write(UINT64_MAX);
        return sepc;
    }
    if ((csr_read_sstatus() & MSTATUS_SPP) == 0u) {
        bad_sstatus++;
    }

    uint32_t m_before = m_irq_count;
    s_irq_count++;
    if (s_irq_count < S_IRQ_TARGET)
        stimecmp_write(time_read() + S_TIMER_INTERVAL);
    else
        stimecmp_write(UINT64_MAX);

    mtimecmp_write(mtime_read() + 1u);
    for (uint32_t wait = 0; wait < NESTED_WAIT_LIMIT; wait++) {
        if (m_irq_count != m_before)
            return sepc;
        asm volatile("nop");
    }

    nested_timeout++;
    mtimecmp_write(UINT64_MAX);
    return sepc;
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc)
{
    if (mcause == MCAUSE_M_TIMER) {
        m_irq_count++;
        mtimecmp_write(UINT64_MAX);
        return mepc;
    }
    if (mcause == MCAUSE_ECALL_S) {
        uint32_t mstatus = csr_read_mstatus();
        mstatus &= ~MSTATUS_MPP_MASK;
        mstatus |= MSTATUS_MPP_M;
        asm volatile("csrw mstatus, %0" :: "r"(mstatus) : "memory");
        return (uint32_t)m_resume_pc;
    }

    bad_mcause = mcause;
    return (uint32_t)m_resume_pc;
}

__attribute__((noinline)) uint32_t epilogue_work(uint32_t seed)
{
    volatile uint32_t frame[8];

    for (uint32_t i = 0; i < 8u; i++)
        frame[i] = seed ^ (0x10203040u + i * 0x11111111u);
    for (uint32_t i = 0; i < 8u; i++)
        seed = (seed << 3) ^ (seed >> 1) ^ frame[i];
    return seed;
}

void s_workload(void)
{
    uint32_t value = 0x13579bdfu;

    while (s_irq_count < S_IRQ_TARGET && bad_scause == 0u &&
        bad_sstatus == 0u && nested_timeout == 0u) {
        value = epilogue_work(value);
    }
    workload_signature = value;
}

#define SAVE_CONTEXT \
    "    addi sp, sp, -128\n" \
    "    sw ra,   0(sp)\n" \
    "    sw gp,   4(sp)\n" \
    "    sw tp,   8(sp)\n" \
    "    sw t0,  12(sp)\n" \
    "    sw t1,  16(sp)\n" \
    "    sw t2,  20(sp)\n" \
    "    sw s0,  24(sp)\n" \
    "    sw s1,  28(sp)\n" \
    "    sw a0,  32(sp)\n" \
    "    sw a1,  36(sp)\n" \
    "    sw a2,  40(sp)\n" \
    "    sw a3,  44(sp)\n" \
    "    sw a4,  48(sp)\n" \
    "    sw a5,  52(sp)\n" \
    "    sw a6,  56(sp)\n" \
    "    sw a7,  60(sp)\n" \
    "    sw s2,  64(sp)\n" \
    "    sw s3,  68(sp)\n" \
    "    sw s4,  72(sp)\n" \
    "    sw s5,  76(sp)\n" \
    "    sw s6,  80(sp)\n" \
    "    sw s7,  84(sp)\n" \
    "    sw s8,  88(sp)\n" \
    "    sw s9,  92(sp)\n" \
    "    sw s10, 96(sp)\n" \
    "    sw s11,100(sp)\n" \
    "    sw t3, 104(sp)\n" \
    "    sw t4, 108(sp)\n" \
    "    sw t5, 112(sp)\n" \
    "    sw t6, 116(sp)\n"

#define RESTORE_CONTEXT \
    "    lw ra,   0(sp)\n" \
    "    lw gp,   4(sp)\n" \
    "    lw tp,   8(sp)\n" \
    "    lw t0,  12(sp)\n" \
    "    lw t1,  16(sp)\n" \
    "    lw t2,  20(sp)\n" \
    "    lw s0,  24(sp)\n" \
    "    lw s1,  28(sp)\n" \
    "    lw a0,  32(sp)\n" \
    "    lw a1,  36(sp)\n" \
    "    lw a2,  40(sp)\n" \
    "    lw a3,  44(sp)\n" \
    "    lw a4,  48(sp)\n" \
    "    lw a5,  52(sp)\n" \
    "    lw a6,  56(sp)\n" \
    "    lw a7,  60(sp)\n" \
    "    lw s2,  64(sp)\n" \
    "    lw s3,  68(sp)\n" \
    "    lw s4,  72(sp)\n" \
    "    lw s5,  76(sp)\n" \
    "    lw s6,  80(sp)\n" \
    "    lw s7,  84(sp)\n" \
    "    lw s8,  88(sp)\n" \
    "    lw s9,  92(sp)\n" \
    "    lw s10, 96(sp)\n" \
    "    lw s11,100(sp)\n" \
    "    lw t3, 104(sp)\n" \
    "    lw t4, 108(sp)\n" \
    "    lw t5, 112(sp)\n" \
    "    lw t6, 116(sp)\n" \
    "    addi sp, sp, 128\n"

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_trap_entry\n"
    "m_trap_entry:\n"
    SAVE_CONTEXT
    "    csrr a0, mcause\n"
    "    csrr a1, mepc\n"
    "    call m_trap_handler\n"
    "    csrw mepc, a0\n"
    RESTORE_CONTEXT
    "    mret\n"
    "\n"
    ".align 2\n"
    ".globl s_trap_entry\n"
    "s_trap_entry:\n"
    SAVE_CONTEXT
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    call s_trap_handler\n"
    "    csrw sepc, a0\n"
    RESTORE_CONTEXT
    "    sret\n"
    "\n"
    ".align 2\n"
    ".globl s_mode_entry\n"
    "s_mode_entry:\n"
    "    call s_workload\n"
    "    ecall\n"
    "1:  j 1b\n"
    "\n"
    ".align 2\n"
    ".globl m_mode_resume\n"
    "m_mode_resume:\n"
    "    la t0, m_resume_ra\n"
    "    lw ra, 0(t0)\n"
    "    ret\n"
);

__attribute__((noinline)) static void enter_supervisor(void)
{
    uint32_t mstatus;
    uintptr_t caller_ra;

    asm volatile("mv %0, ra" : "=r"(caller_ra) :: "memory");
    m_resume_ra = caller_ra;
    m_resume_pc = (uintptr_t)m_mode_resume;
    asm volatile("csrw mtvec, %0" :: "r"((uintptr_t)m_trap_entry) :
        "memory");
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)s_trap_entry) :
        "memory");
    asm volatile("csrs mideleg, %0" :: "r"(MIDELEG_S_TIMER) : "memory");
    asm volatile("csrs mie, %0" :: "r"(MIE_MTIE | MIE_STIE) : "memory");
    stimecmp_write(time_read() + 1u);
    mtimecmp_write(UINT64_MAX);

    mstatus = csr_read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S | MSTATUS_SIE;
    asm volatile("csrw mstatus, %0" :: "r"(mstatus) : "memory");
    asm volatile("csrw mepc, %0" :: "r"((uintptr_t)s_mode_entry) :
        "memory");
    asm volatile("mret" ::: "memory");
}

int main(void)
{
    s_irq_count = 0;
    m_irq_count = 0;
    bad_mcause = 0;
    bad_scause = 0;
    bad_sstatus = 0;
    nested_timeout = 0;
    workload_signature = 0;

    enter_supervisor();
    stimecmp_write(UINT64_MAX);
    mtimecmp_write(UINT64_MAX);

    if (s_irq_count == S_IRQ_TARGET && m_irq_count == S_IRQ_TARGET &&
        bad_mcause == 0u && bad_scause == 0u && bad_sstatus == 0u &&
        nested_timeout == 0u) {
        printf("nested_trap_stress: PASS s_irq=%u m_irq=%u sig=0x%08x\n",
            (unsigned int)s_irq_count, (unsigned int)m_irq_count,
            (unsigned int)workload_signature);
        return 0;
    }

    printf("nested_trap_stress: FAIL s_irq=%u m_irq=%u bad_m=0x%08x "
        "bad_s=0x%08x bad_status=%u timeout=%u sig=0x%08x\n",
        (unsigned int)s_irq_count, (unsigned int)m_irq_count,
        (unsigned int)bad_mcause, (unsigned int)bad_scause,
        (unsigned int)bad_sstatus, (unsigned int)nested_timeout,
        (unsigned int)workload_signature);
    return 1;
}
