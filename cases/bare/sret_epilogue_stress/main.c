#include <stdint.h>
#include <stdio.h>

#define DDR_BASE                0x40000000u
#define EVICT_BASE              0x41000000u
#define L1_SET_STRIDE           8192u

#define PTE_V                   (1u << 0)
#define PTE_R                   (1u << 1)
#define PTE_W                   (1u << 2)
#define PTE_X                   (1u << 3)
#define PTE_A                   (1u << 6)
#define PTE_D                   (1u << 7)
#define SATP_MODE_SV32          (1u << 31)
#define MSTATUS_MPP_MASK        (3u << 11)
#define MSTATUS_MPP_M           (3u << 11)
#define MSTATUS_MPP_S           (1u << 11)
#define MSTATUS_SIE             (1u << 1)
#define MSTATUS_SPP             (1u << 8)
#define MIE_STIE                (1u << 5)
#define MIDELEG_S_TIMER         (1u << 5)
#define MCAUSE_INTERRUPT        0x80000000u
#define SCAUSE_S_TIMER          (MCAUSE_INTERRUPT | 5u)
#define MCAUSE_ECALL_S          9u
#define EPILOGUE_HIT_TARGET     64u
#define IRQ_LIMIT               4096u
#define FRAME_SENTINEL          0xa5963cc3u

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void m_mode_resume(void);
extern void s_workload(void);
extern const char epilogue_window_begin[];
extern const char epilogue_window_end[];
extern const char stress_call_return[];

volatile uintptr_t m_resume_ra;
volatile uintptr_t expected_probe_sp;

static volatile uint32_t irq_count;
static volatile uint32_t epilogue_hit_count;
static volatile uint32_t workload_count;
static volatile uint32_t test_done;
static volatile uint32_t bad_mcause;
static volatile uint32_t bad_scause;
static volatile uint32_t bad_sstatus;
static volatile uint32_t bad_frame;
static volatile uint32_t handler_signature;
static uintptr_t m_resume_pc;
static uint32_t root_pt[1024] __attribute__((aligned(4096)));

static inline uint32_t csr_read_mstatus(void)
{
    uint32_t val;
    asm volatile("csrr %0, mstatus" : "=r"(val));
    return val;
}

static inline uint32_t csr_read_sstatus(void)
{
    uint32_t val;
    asm volatile("csrr %0, sstatus" : "=r"(val));
    return val;
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

    return ((uint64_t)hi0 << 32u) | lo;
}

static void stimecmp_write(uint64_t val)
{
    asm volatile("csrw 0x15d, %0" :: "r"((uint32_t)(val >> 32u)) :
        "memory");
    asm volatile("csrw 0x14d, %0" :: "r"((uint32_t)val) : "memory");
}

static uint32_t pte_from_pa(uintptr_t pa, uint32_t flags)
{
    return ((uint32_t)(pa >> 12u) << 10u) | flags | PTE_V;
}

static void setup_page_table(void)
{
    root_pt[DDR_BASE >> 22u] = pte_from_pa(DDR_BASE,
        PTE_R | PTE_W | PTE_X | PTE_A | PTE_D);
    root_pt[EVICT_BASE >> 22u] = pte_from_pa(EVICT_BASE,
        PTE_R | PTE_W | PTE_X | PTE_A | PTE_D);
}

static void check_frame(const volatile uint32_t *frame, uint32_t sepc)
{
    uintptr_t epilogue_begin = (uintptr_t)epilogue_window_begin;
    uintptr_t epilogue_end = (uintptr_t)epilogue_window_end;

    if (frame[0] != sepc || frame[34] != FRAME_SENTINEL) {
        bad_frame++;
    }
    if (sepc >= epilogue_begin && sepc < epilogue_end &&
        (frame[1] != (uint32_t)(uintptr_t)stress_call_return ||
        frame[2] != (uint32_t)expected_probe_sp)) {
        bad_frame++;
    }
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval,
    uintptr_t frame_addr)
{
    volatile uint32_t *frame = (volatile uint32_t *)frame_addr;
    uintptr_t set_offset = frame_addr & (L1_SET_STRIDE - 1u);
    volatile uint32_t *conflict0 =
        (volatile uint32_t *)(EVICT_BASE + set_offset);
    volatile uint32_t *conflict1 =
        (volatile uint32_t *)(EVICT_BASE + set_offset + L1_SET_STRIDE);
    volatile uint32_t *conflict2 =
        (volatile uint32_t *)(EVICT_BASE + set_offset + 2u * L1_SET_STRIDE);

    (void)stval;
    if (scause != SCAUSE_S_TIMER) {
        bad_scause = scause;
        test_done = 1u;
        stimecmp_write(UINT64_MAX);
        return sepc;
    }
    if ((csr_read_sstatus() & MSTATUS_SPP) == 0u) {
        bad_sstatus++;
    }

    irq_count++;
    check_frame(frame, sepc);
    *conflict0 = irq_count ^ 0x13579bdfu;
    *conflict1 = irq_count ^ 0x2468ace0u;
    *conflict2 = irq_count ^ 0x5a5aa5a5u;
    handler_signature = (handler_signature << 5) ^
        (handler_signature >> 2) ^ *conflict0 ^ *conflict1 ^ *conflict2;
    check_frame(frame, sepc);

    if (sepc >= (uintptr_t)epilogue_window_begin &&
        sepc < (uintptr_t)epilogue_window_end) {
        epilogue_hit_count++;
    }
    if (epilogue_hit_count >= EPILOGUE_HIT_TARGET ||
        irq_count >= IRQ_LIMIT || bad_frame != 0u) {
        test_done = 1u;
        stimecmp_write(UINT64_MAX);
    } else {
        stimecmp_write(time_read() + 1u + (irq_count & 7u));
    }
    return sepc;
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc)
{
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

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_trap_entry\n"
    "m_trap_entry:\n"
    "    addi sp, sp, -64\n"
    "    sw ra,  0(sp)\n"
    "    sw t0,  4(sp)\n"
    "    sw t1,  8(sp)\n"
    "    sw t2, 12(sp)\n"
    "    sw a0, 16(sp)\n"
    "    sw a1, 20(sp)\n"
    "    sw a2, 24(sp)\n"
    "    sw a3, 28(sp)\n"
    "    sw a4, 32(sp)\n"
    "    sw a5, 36(sp)\n"
    "    sw a6, 40(sp)\n"
    "    sw a7, 44(sp)\n"
    "    sw t3, 48(sp)\n"
    "    sw t4, 52(sp)\n"
    "    sw t5, 56(sp)\n"
    "    sw t6, 60(sp)\n"
    "    csrr a0, mcause\n"
    "    csrr a1, mepc\n"
    "    call m_trap_handler\n"
    "    csrw mepc, a0\n"
    "    lw ra,  0(sp)\n"
    "    lw t0,  4(sp)\n"
    "    lw t1,  8(sp)\n"
    "    lw t2, 12(sp)\n"
    "    lw a0, 16(sp)\n"
    "    lw a1, 20(sp)\n"
    "    lw a2, 24(sp)\n"
    "    lw a3, 28(sp)\n"
    "    lw a4, 32(sp)\n"
    "    lw a5, 36(sp)\n"
    "    lw a6, 40(sp)\n"
    "    lw a7, 44(sp)\n"
    "    lw t3, 48(sp)\n"
    "    lw t4, 52(sp)\n"
    "    lw t5, 56(sp)\n"
    "    lw t6, 60(sp)\n"
    "    addi sp, sp, 64\n"
    "    mret\n"
    "\n"
    ".align 2\n"
    ".globl m_mode_resume\n"
    "m_mode_resume:\n"
    "    la t0, m_resume_ra\n"
    "    lw ra, 0(t0)\n"
    "    ret\n"
    "\n"
    ".align 2\n"
    ".globl s_trap_entry\n"
    "s_trap_entry:\n"
    "    addi sp, sp, -144\n"
    "    sw ra,   4(sp)\n"
    "    sw gp,  12(sp)\n"
    "    sw tp,  16(sp)\n"
    "    sw t0,  20(sp)\n"
    "    sw t1,  24(sp)\n"
    "    sw t2,  28(sp)\n"
    "    sw s0,  32(sp)\n"
    "    sw s1,  36(sp)\n"
    "    sw a0,  40(sp)\n"
    "    sw a1,  44(sp)\n"
    "    sw a2,  48(sp)\n"
    "    sw a3,  52(sp)\n"
    "    sw a4,  56(sp)\n"
    "    sw a5,  60(sp)\n"
    "    sw a6,  64(sp)\n"
    "    sw a7,  68(sp)\n"
    "    sw s2,  72(sp)\n"
    "    sw s3,  76(sp)\n"
    "    sw s4,  80(sp)\n"
    "    sw s5,  84(sp)\n"
    "    sw s6,  88(sp)\n"
    "    sw s7,  92(sp)\n"
    "    sw s8,  96(sp)\n"
    "    sw s9, 100(sp)\n"
    "    sw s10,104(sp)\n"
    "    sw s11,108(sp)\n"
    "    sw t3, 112(sp)\n"
    "    sw t4, 116(sp)\n"
    "    sw t5, 120(sp)\n"
    "    sw t6, 124(sp)\n"
    "    addi t0, sp, 144\n"
    "    sw t0,   8(sp)\n"
    "    csrr t0, sepc\n"
    "    sw t0,   0(sp)\n"
    "    csrr t0, sstatus\n"
    "    sw t0, 128(sp)\n"
    "    csrr t0, stval\n"
    "    sw t0, 132(sp)\n"
    "    li t0, 0xa5963cc3\n"
    "    sw t0, 136(sp)\n"
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    csrr a2, stval\n"
    "    mv a3, sp\n"
    "    call s_trap_handler\n"
    "    mv t0, a0\n"
    "    lw t1, 128(sp)\n"
    "    csrw sstatus, t1\n"
    "    csrw sepc, t0\n"
    "    lw ra,   4(sp)\n"
    "    lw gp,  12(sp)\n"
    "    lw tp,  16(sp)\n"
    "    lw t0,  20(sp)\n"
    "    lw t1,  24(sp)\n"
    "    lw t2,  28(sp)\n"
    "    lw s0,  32(sp)\n"
    "    lw s1,  36(sp)\n"
    "    lw a0,  40(sp)\n"
    "    lw a1,  44(sp)\n"
    "    lw a2,  48(sp)\n"
    "    lw a3,  52(sp)\n"
    "    lw a4,  56(sp)\n"
    "    lw a5,  60(sp)\n"
    "    lw a6,  64(sp)\n"
    "    lw a7,  68(sp)\n"
    "    lw s2,  72(sp)\n"
    "    lw s3,  76(sp)\n"
    "    lw s4,  80(sp)\n"
    "    lw s5,  84(sp)\n"
    "    lw s6,  88(sp)\n"
    "    lw s7,  92(sp)\n"
    "    lw s8,  96(sp)\n"
    "    lw s9, 100(sp)\n"
    "    lw s10,104(sp)\n"
    "    lw s11,108(sp)\n"
    "    lw t3, 112(sp)\n"
    "    lw t4, 116(sp)\n"
    "    lw t5, 120(sp)\n"
    "    lw t6, 124(sp)\n"
    "    lw sp,   8(sp)\n"
    "    sret\n"
    "\n"
    ".align 2\n"
    ".globl epilogue_probe\n"
    "epilogue_probe:\n"
    "    addi sp, sp, -16\n"
    "    sw ra, 12(sp)\n"
    "    sw s0,  8(sp)\n"
    "    xor s0, s0, a0\n"
    ".globl epilogue_window_begin\n"
    "epilogue_window_begin:\n"
    "    lw ra, 12(sp)\n"
    "    lw s0,  8(sp)\n"
    "    addi sp, sp, 16\n"
    ".globl epilogue_window_end\n"
    "epilogue_window_end:\n"
    "    ret\n"
    "\n"
    ".align 2\n"
    ".globl s_workload\n"
    "s_workload:\n"
    "    addi sp, sp, -16\n"
    "    sw ra, 12(sp)\n"
    "    sw s0,  8(sp)\n"
    "    addi t0, sp, -16\n"
    "    la t1, expected_probe_sp\n"
    "    sw t0, 0(t1)\n"
    "    li s0, 0\n"
    "3:\n"
    "    mv a0, s0\n"
    "    call epilogue_probe\n"
    ".globl stress_call_return\n"
    "stress_call_return:\n"
    "    addi s0, s0, 1\n"
    "    la t0, test_done\n"
    "    lw t0, 0(t0)\n"
    "    beqz t0, 3b\n"
    "    la t0, workload_count\n"
    "    sw s0, 0(t0)\n"
    "    lw ra, 12(sp)\n"
    "    lw s0,  8(sp)\n"
    "    addi sp, sp, 16\n"
    "    ret\n"
    "\n"
    ".align 2\n"
    ".globl s_mode_entry\n"
    "s_mode_entry:\n"
    "    call s_workload\n"
    "    ecall\n"
    "4:  j 4b\n"
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
    asm volatile("csrs mie, %0" :: "r"(MIE_STIE) : "memory");
    asm volatile("csrw satp, %0\nsfence.vma" ::
        "r"(SATP_MODE_SV32 | ((uint32_t)(uintptr_t)root_pt >> 12u)) :
        "memory");
    stimecmp_write(time_read() + 1u);

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
    irq_count = 0;
    epilogue_hit_count = 0;
    workload_count = 0;
    test_done = 0;
    bad_mcause = 0;
    bad_scause = 0;
    bad_sstatus = 0;
    bad_frame = 0;
    handler_signature = 0x13579bdfu;
    setup_page_table();

    enter_supervisor();
    stimecmp_write(UINT64_MAX);

    if (epilogue_hit_count >= EPILOGUE_HIT_TARGET &&
        irq_count <= IRQ_LIMIT && workload_count != 0u &&
        bad_mcause == 0u && bad_scause == 0u && bad_sstatus == 0u &&
        bad_frame == 0u) {
        printf("sret_epilogue_stress: PASS irq=%u epilogue=%u loops=%u "
            "signature=0x%08x\n", (unsigned int)irq_count,
            (unsigned int)epilogue_hit_count,
            (unsigned int)workload_count,
            (unsigned int)handler_signature);
        return 0;
    }

    printf("sret_epilogue_stress: FAIL irq=%u epilogue=%u loops=%u "
        "bad_m=0x%08x bad_s=0x%08x bad_status=%u bad_frame=%u "
        "signature=0x%08x\n", (unsigned int)irq_count,
        (unsigned int)epilogue_hit_count, (unsigned int)workload_count,
        (unsigned int)bad_mcause, (unsigned int)bad_scause,
        (unsigned int)bad_sstatus, (unsigned int)bad_frame,
        (unsigned int)handler_signature);
    return 1;
}
