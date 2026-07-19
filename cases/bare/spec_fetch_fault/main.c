#include <stdint.h>
#include <stdio.h>

#define DDR_BASE                0x40000000u
#define BAD_TARGET              0x20000000u
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
#define MSTATUS_SPP             (1u << 8)
#define MEDELEG_INST_PAGE_FAULT (1u << 12)
#define SCAUSE_INST_PAGE_FAULT  12u
#define MCAUSE_ECALL_S          9u
#define ITERATION_NUM           128u

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void m_mode_resume(void);
extern const char jalr_site[];
extern const char valid_target[];

volatile uintptr_t m_resume_ra;

static volatile uint32_t expected_fault_count;
static volatile uint32_t wrong_path_fault_count;
static volatile uint32_t completed_count;
static volatile uint32_t bad_mcause;
static volatile uint32_t bad_scause;
static volatile uint32_t bad_stval;
static volatile uint32_t bad_sstatus;
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

static uint32_t pte_from_pa(uintptr_t pa, uint32_t flags)
{
    return ((uint32_t)(pa >> 12u) << 10u) | flags | PTE_V;
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval,
    volatile uint32_t *frame)
{
    if (scause != SCAUSE_INST_PAGE_FAULT) {
        bad_scause = scause;
    }
    if (stval != BAD_TARGET) {
        bad_stval = stval;
    }
    if ((csr_read_sstatus() & MSTATUS_SPP) == 0u) {
        bad_sstatus++;
    }

    if (frame[1] == BAD_TARGET) {
        expected_fault_count++;
    } else {
        wrong_path_fault_count++;
    }
    frame[1] = (uint32_t)(uintptr_t)valid_target;
    return (uint32_t)(uintptr_t)jalr_site;
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
    "    addi sp, sp, -32\n"
    "    sw ra,  0(sp)\n"
    "    sw t0,  4(sp)\n"
    "    sw t1,  8(sp)\n"
    "    sw t2, 12(sp)\n"
    "    sw a0, 16(sp)\n"
    "    sw a1, 20(sp)\n"
    "    sw a2, 24(sp)\n"
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
    "    addi sp, sp, 32\n"
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
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    csrr a2, stval\n"
    "    mv a3, sp\n"
    "    call s_trap_handler\n"
    "    csrw sepc, a0\n"
    "    lw t0, 128(sp)\n"
    "    csrw sstatus, t0\n"
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
    ".globl s_mode_entry\n"
    "s_mode_entry:\n"
    "    li ra, 0x20000000\n"
    ".globl jalr_site\n"
    "jalr_site:\n"
    "    ret\n"
    ".globl valid_target\n"
    "valid_target:\n"
    "    la t0, completed_count\n"
    "    lw t1, 0(t0)\n"
    "    addi t1, t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    li t2, 128\n"
    "    bgeu t1, t2, 1f\n"
    "    li ra, 0x20000000\n"
    "    j jalr_site\n"
    "1:  ecall\n"
    "2:  j 2b\n"
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
    asm volatile("csrs medeleg, %0" ::
        "r"(MEDELEG_INST_PAGE_FAULT) : "memory");
    asm volatile("csrw satp, %0\nsfence.vma" ::
        "r"(SATP_MODE_SV32 | ((uint32_t)(uintptr_t)root_pt >> 12u)) :
        "memory");

    mstatus = csr_read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    asm volatile("csrw mstatus, %0" :: "r"(mstatus) : "memory");
    asm volatile("csrw mepc, %0" :: "r"((uintptr_t)s_mode_entry) :
        "memory");
    asm volatile("mret" ::: "memory");
}

int main(void)
{
    expected_fault_count = 0;
    wrong_path_fault_count = 0;
    completed_count = 0;
    bad_mcause = 0;
    bad_scause = 0;
    bad_stval = 0;
    bad_sstatus = 0;
    root_pt[DDR_BASE >> 22u] = pte_from_pa(DDR_BASE,
        PTE_R | PTE_W | PTE_X | PTE_A | PTE_D);

    enter_supervisor();

    if (expected_fault_count == ITERATION_NUM &&
        completed_count == ITERATION_NUM && wrong_path_fault_count == 0u &&
        bad_mcause == 0u && bad_scause == 0u && bad_stval == 0u &&
        bad_sstatus == 0u) {
        printf("spec_fetch_fault: PASS expected=%u completed=%u\n",
            (unsigned int)expected_fault_count,
            (unsigned int)completed_count);
        return 0;
    }

    printf("spec_fetch_fault: FAIL expected=%u wrong_path=%u completed=%u "
        "bad_m=0x%08x bad_s=0x%08x bad_stval=0x%08x bad_status=%u\n",
        (unsigned int)expected_fault_count,
        (unsigned int)wrong_path_fault_count,
        (unsigned int)completed_count, (unsigned int)bad_mcause,
        (unsigned int)bad_scause, (unsigned int)bad_stval,
        (unsigned int)bad_sstatus);
    return 1;
}
