#include <stdint.h>
#include <stdio.h>

#define MSTATUS_MPP_MASK    (3u << 11)
#define MSTATUS_MPP_M       (3u << 11)
#define MSTATUS_MPP_S       (1u << 11)
#define MEDELEG_ECALL_U     (1u << 8)
#define MCAUSE_ECALL_U      8u
#define MCAUSE_ECALL_S      9u
#define S_TRAP_TO_M         0xffffffffu

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void m_mode_resume(void);

static volatile uint32_t s_entered;
static volatile uint32_t u_entered;
static volatile uint32_t u_ecall_to_s;
static volatile uint32_t s_ecall_to_m;
static volatile uint32_t m_returned;
static volatile uint32_t unexpected_u_return;
static volatile uint32_t bad_mcause;
static volatile uint32_t bad_scause;
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

static inline void csr_write_mepc(uintptr_t val)
{
    asm volatile("csrw mepc, %0" :: "r"(val) : "memory");
}

static inline void csr_write_mtvec(uintptr_t val)
{
    asm volatile("csrw mtvec, %0" :: "r"(val) : "memory");
}

static inline void csr_write_stvec(uintptr_t val)
{
    asm volatile("csrw stvec, %0" :: "r"(val) : "memory");
}

static inline void csr_set_medeleg(uint32_t val)
{
    asm volatile("csrs medeleg, %0" :: "r"(val) : "memory");
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval)
{
    (void)sepc;
    (void)stval;

    if (scause == MCAUSE_ECALL_U) {
        u_ecall_to_s = 1;
        return S_TRAP_TO_M;
    }

    bad_scause = scause;
    return S_TRAP_TO_M;
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mepc;
    (void)mtval;

    if (mcause == MCAUSE_ECALL_S) {
        uint32_t mstatus = csr_read_mstatus();
        mstatus &= ~MSTATUS_MPP_MASK;
        mstatus |= MSTATUS_MPP_M;
        csr_write_mstatus(mstatus);

        s_ecall_to_m = 1;
        return (uint32_t)m_resume_pc;
    }

    bad_mcause = mcause;
    return (uint32_t)m_resume_pc;
}

__attribute__((noinline)) static void enter_supervisor(void)
{
    m_resume_pc = (uintptr_t)m_mode_resume;

    csr_write_mtvec((uintptr_t)m_trap_entry);
    csr_write_stvec((uintptr_t)s_trap_entry);
    csr_set_medeleg(MEDELEG_ECALL_U);

    uint32_t mstatus = csr_read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    csr_write_mepc((uintptr_t)s_mode_entry);

    asm volatile("mret" ::: "memory");
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_trap_entry\n"
    ".type m_trap_entry,@function\n"
    "m_trap_entry:\n"
    "    addi sp, sp, -32\n"
    "    sw ra,  0(sp)\n"
    "    sw t0,  4(sp)\n"
    "    sw a0,  8(sp)\n"
    "    sw a1, 12(sp)\n"
    "    sw a2, 16(sp)\n"
    "    csrr a0, mcause\n"
    "    csrr a1, mepc\n"
    "    csrr a2, mtval\n"
    "    call m_trap_handler\n"
    "    csrw mepc, a0\n"
    "    lw ra,  0(sp)\n"
    "    lw t0,  4(sp)\n"
    "    lw a0,  8(sp)\n"
    "    lw a1, 12(sp)\n"
    "    lw a2, 16(sp)\n"
    "    addi sp, sp, 32\n"
    "    mret\n"
    "\n"
    ".align 2\n"
    ".globl m_mode_resume\n"
    ".type m_mode_resume,@function\n"
    "m_mode_resume:\n"
    "    ret\n"
    "\n"
    ".align 2\n"
    ".globl s_trap_entry\n"
    ".type s_trap_entry,@function\n"
    "s_trap_entry:\n"
    "    addi sp, sp, -32\n"
    "    sw ra,  0(sp)\n"
    "    sw t0,  4(sp)\n"
    "    sw t1,  8(sp)\n"
    "    sw a0, 12(sp)\n"
    "    sw a1, 16(sp)\n"
    "    sw a2, 20(sp)\n"
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    csrr a2, stval\n"
    "    call s_trap_handler\n"
    "    mv t0, a0\n"
    "    li t1, -1\n"
    "    beq t0, t1, 1f\n"
    "    csrw sepc, t0\n"
    "    lw ra,  0(sp)\n"
    "    lw t0,  4(sp)\n"
    "    lw t1,  8(sp)\n"
    "    lw a0, 12(sp)\n"
    "    lw a1, 16(sp)\n"
    "    lw a2, 20(sp)\n"
    "    addi sp, sp, 32\n"
    "    sret\n"
    "1:\n"
    "    lw ra,  0(sp)\n"
    "    lw t0,  4(sp)\n"
    "    lw t1,  8(sp)\n"
    "    lw a0, 12(sp)\n"
    "    lw a1, 16(sp)\n"
    "    lw a2, 20(sp)\n"
    "    addi sp, sp, 32\n"
    "    ecall\n"
    "    j .\n"
    "\n"
    ".align 2\n"
    ".globl s_mode_entry\n"
    ".type s_mode_entry,@function\n"
    "s_mode_entry:\n"
    "    la t0, s_entered\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    la t0, u_mode_entry\n"
    "    csrw sepc, t0\n"
    "    sret\n"
    "\n"
    ".align 2\n"
    "u_mode_entry:\n"
    "    la t0, u_entered\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    ecall\n"
    "    la t0, unexpected_u_return\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    j .\n"
);

int main(void)
{
    s_entered = 0;
    u_entered = 0;
    u_ecall_to_s = 0;
    s_ecall_to_m = 0;
    m_returned = 0;
    unexpected_u_return = 0;
    bad_mcause = 0;
    bad_scause = 0;

    enter_supervisor();
    m_returned = 1;

    if (s_entered == 1u &&
        u_entered == 1u &&
        u_ecall_to_s == 1u &&
        s_ecall_to_m == 1u &&
        m_returned == 1u &&
        unexpected_u_return == 0u &&
        bad_mcause == 0u &&
        bad_scause == 0u) {
        printf("priv_switch: PASS\n");
        return 0;
    }

    printf("priv_switch: FAIL s=%u u=%u u2s=%u s2m=%u mr=%u badm=0x%08x bads=0x%08x ur=%u\n",
        (unsigned int)s_entered,
        (unsigned int)u_entered,
        (unsigned int)u_ecall_to_s,
        (unsigned int)s_ecall_to_m,
        (unsigned int)m_returned,
        (unsigned int)bad_mcause,
        (unsigned int)bad_scause,
        (unsigned int)unexpected_u_return);
    return 1;
}
