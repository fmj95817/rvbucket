#include <stdint.h>
#include <stdio.h>

#define FIRMWARE_BASE    0x40000000u
#define USER_CODE_PA     0x40400000u
#define USER_CODE_VA     0x00177000u
#define USER_DATA_VA     0x00178000u
#define USER_DATA_PA     0x40401000u
#define KERNEL_TRAP_VA   0xc0400000u
#define KERNEL_TRAP_PA   0x40800000u
#define PTE_V            (1u << 0)
#define PTE_R            (1u << 1)
#define PTE_W            (1u << 2)
#define PTE_X            (1u << 3)
#define PTE_U            (1u << 4)
#define PTE_A            (1u << 6)
#define PTE_D            (1u << 7)
#define SATP_MODE_SV32   (1u << 31)
#define MSTATUS_MPP_MASK (3u << 11)
#define MSTATUS_MPP_M    (3u << 11)
#define MSTATUS_MPP_S    (1u << 11)
#define SSTATUS_SPP      (1u << 8)
#define MCAUSE_ECALL_U   8u
#define MCAUSE_ECALL_S   9u
#define TRAP_TARGET      32u

extern uint32_t user_payload_start[];
extern uint32_t user_payload_end[];
extern uint32_t kernel_trap_payload_start[];
extern uint32_t kernel_trap_payload_end[];
extern void s_mode_entry(void);
extern void s_mode_finish(void);
extern void m_mode_resume(void);

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t user_l0_pt[1024] __attribute__((aligned(4096)));
static volatile uint32_t trap_count;
static volatile uint32_t bad_scause;
static volatile uint32_t bad_mcause;
static uintptr_t m_resume_pc;

static inline uint32_t csr_read_mstatus(void)
{
    uint32_t val;
    asm volatile("csrr %0, mstatus" : "=r"(val));
    return val;
}

static inline void csr_write_mstatus(uint32_t val)
{
    asm volatile("csrw mstatus, %0" :: "r"(val) : "memory");
}

static uint32_t superpage_pte(uint32_t pa, uint32_t flags)
{
    return ((pa >> 12u) << 10u) | flags | PTE_V | PTE_A | PTE_D;
}

uint32_t trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mepc;
    (void)mtval;

    if (mcause == MCAUSE_ECALL_S) {
        uint32_t mstatus = csr_read_mstatus();
        mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_M;
        csr_write_mstatus(mstatus);
        return (uint32_t)m_resume_pc;
    }

    bad_mcause = mcause;
    return (uint32_t)m_resume_pc;
}

static void setup_page_table(void)
{
    root_pt[FIRMWARE_BASE >> 22u] =
        superpage_pte(FIRMWARE_BASE, PTE_R | PTE_W | PTE_X);
    root_pt[USER_CODE_VA >> 22u] =
        (((uintptr_t)user_l0_pt >> 12u) << 10u) | PTE_V;
    user_l0_pt[(USER_CODE_VA >> 12u) & 0x3ffu] =
        superpage_pte(USER_CODE_PA, PTE_R | PTE_X | PTE_U);
    user_l0_pt[(USER_DATA_VA >> 12u) & 0x3ffu] =
        superpage_pte(USER_DATA_PA, PTE_R | PTE_W | PTE_U);
    root_pt[KERNEL_TRAP_VA >> 22u] =
        superpage_pte(KERNEL_TRAP_PA, PTE_R | PTE_X);
}

static void copy_user_payload(void)
{
    volatile uint32_t *dst = (volatile uint32_t *)USER_CODE_PA;
    const uint32_t *src = user_payload_start;
    uint32_t word_num = (uint32_t)(user_payload_end - user_payload_start);

    for (uint32_t i = 0; i < word_num; i++) {
        dst[i] = src[i];
    }

    dst = (volatile uint32_t *)KERNEL_TRAP_PA;
    src = kernel_trap_payload_start;
    word_num = (uint32_t)(kernel_trap_payload_end -
        kernel_trap_payload_start);
    for (uint32_t i = 0; i < word_num; i++) {
        dst[i] = src[i];
    }

    *(volatile uint32_t *)USER_DATA_PA = 0u;
    asm volatile(".word 0x0000100f" ::: "memory");
}

__attribute__((noinline)) static void enter_user(void)
{
    uint32_t satp = SATP_MODE_SV32 | ((uintptr_t)root_pt >> 12u);
    uint32_t mstatus = csr_read_mstatus();

    m_resume_pc = (uintptr_t)m_mode_resume;
    asm volatile("csrw stvec, %0" :: "r"(KERNEL_TRAP_VA) : "memory");
    asm volatile("csrw medeleg, %0" :: "r"(1u << MCAUSE_ECALL_U) : "memory");
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(satp) : "memory");
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    asm volatile("csrw mepc, %0" :: "r"(s_mode_entry) : "memory");
    asm volatile("mret" ::: "memory");
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl s_mode_entry\n"
    ".type s_mode_entry,@function\n"
    "s_mode_entry:\n"
    "    li t0, 0x100\n"
    "    csrc sstatus, t0\n"
    "    li t0, 0x00177000\n"
    "    csrw sepc, t0\n"
    "    sret\n"
    "\n"
    ".align 2\n"
    ".globl s_mode_finish\n"
    ".type s_mode_finish,@function\n"
    "s_mode_finish:\n"
    "    ecall\n"
    "1:\n"
    "    j 1b\n"
    "\n"
    ".align 2\n"
    ".globl m_mode_resume\n"
    ".type m_mode_resume,@function\n"
    "m_mode_resume:\n"
    "    ret\n"
    "\n"
    ".section .rodata\n"
    ".align 2\n"
    ".globl user_payload_start\n"
    "user_payload_start:\n"
    "    li t0, 0x00178000\n"
    "    lw t1, 0(t0)\n"
    "    addi t1, t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    ecall\n"
    "    .rept 16\n"
    "    nop\n"
    "    .endr\n"
    "    j user_payload_start\n"
    ".globl user_payload_end\n"
    "user_payload_end:\n"
    "\n"
    ".align 2\n"
    ".globl kernel_trap_payload_start\n"
    "kernel_trap_payload_start:\n"
    "    csrr t0, scause\n"
    "    li t1, 8\n"
    "    bne t0, t1, 2f\n"
    "    lui t0, %hi(trap_count)\n"
    "    lw t1, %lo(trap_count)(t0)\n"
    "    addi t1, t1, 1\n"
    "    sw t1, %lo(trap_count)(t0)\n"
    "    li t2, 32\n"
    "    bgeu t1, t2, 3f\n"
    "    csrr t0, sepc\n"
    "    addi t0, t0, 4\n"
    "    csrw sepc, t0\n"
    "    sret\n"
    "2:\n"
    "    lui t1, %hi(bad_scause)\n"
    "    sw t0, %lo(bad_scause)(t1)\n"
    "3:\n"
    "    li t0, 0x100\n"
    "    csrs sstatus, t0\n"
    "    lui t0, %hi(s_mode_finish)\n"
    "    addi t0, t0, %lo(s_mode_finish)\n"
    "    csrw sepc, t0\n"
    "    sret\n"
    ".globl kernel_trap_payload_end\n"
    "kernel_trap_payload_end:\n"
    ".section .text\n"
);

int main(void)
{
    trap_count = 0;
    bad_scause = 0;
    bad_mcause = 0;

    asm volatile("csrci mstatus, 8\ncsrw mie, zero" ::: "memory");
    setup_page_table();
    copy_user_payload();
    enter_user();

    if (trap_count == TRAP_TARGET && bad_scause == 0u && bad_mcause == 0u &&
        *(volatile uint32_t *)USER_DATA_PA == TRAP_TARGET) {
        printf("mmu_i_ost_collision: PASS\n");
        return 0;
    }

    printf("mmu_i_ost_collision: FAIL traps=%u scause=0x%08x mcause=0x%08x\n",
        (unsigned int)trap_count, (unsigned int)bad_scause,
        (unsigned int)bad_mcause);
    return 1;
}
