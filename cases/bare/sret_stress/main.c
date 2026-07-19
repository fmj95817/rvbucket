#include <stdint.h>
#include <stdio.h>

#define DDR_BASE                0x40000000u
#define U_CODE_VA               0x10000000u
#define EVICT_BASE              0x41000000u
#define L1_SET_STRIDE           8192u

#define PTE_V                   (1u << 0)
#define PTE_R                   (1u << 1)
#define PTE_W                   (1u << 2)
#define PTE_X                   (1u << 3)
#define PTE_U                   (1u << 4)
#define PTE_A                   (1u << 6)
#define PTE_D                   (1u << 7)
#define SATP_MODE_SV32          (1u << 31)
#define MSTATUS_MPP_MASK        (3u << 11)
#define MSTATUS_MPP_M           (3u << 11)
#define MSTATUS_MPP_S           (1u << 11)
#define MSTATUS_SPP             (1u << 8)
#define MEDELEG_ECALL_U         (1u << 8)
#define MCAUSE_ECALL_U          8u
#define MCAUSE_ECALL_S          9u
#define S_TRAP_TO_M             UINT32_MAX
#define ITERATION_NUM           10000u
#define FRAME_SENTINEL          0x5a96c33cu

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void m_mode_resume(void);

volatile uintptr_t m_resume_ra;
volatile uintptr_t expected_user_sp;

static volatile uint32_t iteration_count;
static volatile uint32_t bad_mcause;
static volatile uint32_t bad_scause;
static volatile uint32_t bad_sstatus;
static volatile uint32_t bad_frame;
static volatile uint32_t handler_signature;
static uintptr_t m_resume_pc;
static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t user_l0_pt[1024] __attribute__((aligned(4096)));
static uint32_t user_code_page[1024] __attribute__((aligned(4096)));

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

static inline void csr_write_satp(uint32_t val)
{
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(val) : "memory");
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
    root_pt[U_CODE_VA >> 22u] = pte_from_pa((uintptr_t)user_l0_pt, 0u);
    user_l0_pt[(U_CODE_VA >> 12u) & 0x3ffu] =
        pte_from_pa((uintptr_t)user_code_page,
            PTE_R | PTE_X | PTE_U | PTE_A | PTE_D);

    /* ecall; ret; j -8. The ret consumes ra restored from the trap frame. */
    user_code_page[0] = 0x00000073u;
    user_code_page[1] = 0x00008067u;
    user_code_page[2] = 0xff9ff06fu;
    asm volatile(".word 0x0000100f" ::: "memory");
}

static void check_frame(const volatile uint32_t *frame, uint32_t sepc)
{
    if (frame[0] != sepc || frame[1] != U_CODE_VA + 8u ||
        frame[2] != (uint32_t)expected_user_sp ||
        frame[34] != FRAME_SENTINEL) {
        bad_frame++;
    }
}

__attribute__((noinline)) static uint32_t s_handler_body(uint32_t sepc,
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

    check_frame(frame, sepc);
    *conflict0 = iteration_count ^ 0x13579bdfu;
    *conflict1 = iteration_count ^ 0x2468ace0u;
    *conflict2 = iteration_count ^ 0x5a5aa5a5u;
    handler_signature = (handler_signature << 5) ^
        (handler_signature >> 2) ^ *conflict0 ^ *conflict1 ^ *conflict2;
    check_frame(frame, sepc);
    return sepc + 4u;
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval,
    uintptr_t frame_addr)
{
    (void)stval;

    if (scause != MCAUSE_ECALL_U || sepc != U_CODE_VA) {
        bad_scause = scause;
        return S_TRAP_TO_M;
    }
    if ((csr_read_sstatus() & MSTATUS_SPP) != 0u) {
        bad_sstatus++;
        return S_TRAP_TO_M;
    }

    iteration_count++;
    if (iteration_count >= ITERATION_NUM) {
        check_frame((volatile uint32_t *)frame_addr, sepc);
        return S_TRAP_TO_M;
    }
    return s_handler_body(sepc, frame_addr);
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mtval;

    if (mcause == MCAUSE_ECALL_S) {
        uint32_t mstatus = csr_read_mstatus();
        mstatus &= ~MSTATUS_MPP_MASK;
        mstatus |= MSTATUS_MPP_M;
        csr_write_mstatus(mstatus);
        return (uint32_t)m_resume_pc;
    }

    bad_mcause = mcause;
    return (uint32_t)m_resume_pc;
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_trap_entry\n"
    ".type m_trap_entry,@function\n"
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
    "    csrr a2, mtval\n"
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
    ".type m_mode_resume,@function\n"
    "m_mode_resume:\n"
    "    la t0, m_resume_ra\n"
    "    lw ra, 0(t0)\n"
    "    ret\n"
    "\n"
    ".align 2\n"
    ".globl s_trap_entry\n"
    ".type s_trap_entry,@function\n"
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
    "    li t0, 0x5a96c33c\n"
    "    sw t0, 136(sp)\n"
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    csrr a2, stval\n"
    "    mv a3, sp\n"
    "    call s_trap_handler\n"
    "    mv t0, a0\n"
    "    li t1, -1\n"
    "    beq t0, t1, 1f\n"
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
    "1:\n"
    "    lw sp,   8(sp)\n"
    "    ecall\n"
    "2:  j 2b\n"
    "\n"
    ".align 2\n"
    ".globl s_mode_entry\n"
    ".type s_mode_entry,@function\n"
    "s_mode_entry:\n"
    "    la t0, expected_user_sp\n"
    "    sw sp, 0(t0)\n"
    "    li ra, 0x10000008\n"
    "    li t0, 0x10000000\n"
    "    csrw sepc, t0\n"
    "    csrr t1, sstatus\n"
    "    li t2, 0x100\n"
    "    not t2, t2\n"
    "    and t1, t1, t2\n"
    "    csrw sstatus, t1\n"
    "    sret\n"
);

__attribute__((noinline)) static void enter_supervisor(void)
{
    uint32_t mstatus;
    uintptr_t caller_ra;

    asm volatile("mv %0, ra" : "=r"(caller_ra) :: "memory");
    m_resume_ra = caller_ra;
    m_resume_pc = (uintptr_t)m_mode_resume;
    csr_write_mtvec((uintptr_t)m_trap_entry);
    csr_write_stvec((uintptr_t)s_trap_entry);
    csr_set_medeleg(MEDELEG_ECALL_U);
    csr_write_satp(SATP_MODE_SV32 | ((uint32_t)(uintptr_t)root_pt >> 12u));

    mstatus = csr_read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    csr_write_mepc((uintptr_t)s_mode_entry);
    asm volatile("mret" ::: "memory");
}

int main(void)
{
    iteration_count = 0;
    bad_mcause = 0;
    bad_scause = 0;
    bad_sstatus = 0;
    bad_frame = 0;
    handler_signature = 0x13579bdfu;
    setup_page_table();

    enter_supervisor();

    if (iteration_count == ITERATION_NUM && bad_mcause == 0u &&
        bad_scause == 0u && bad_sstatus == 0u && bad_frame == 0u) {
        printf("sret_stress: PASS iterations=%u signature=0x%08x\n",
            (unsigned int)iteration_count,
            (unsigned int)handler_signature);
        return 0;
    }

    printf("sret_stress: FAIL iterations=%u bad_m=0x%08x "
        "bad_s=0x%08x bad_status=%u bad_frame=%u signature=0x%08x\n",
        (unsigned int)iteration_count, (unsigned int)bad_mcause,
        (unsigned int)bad_scause, (unsigned int)bad_sstatus,
        (unsigned int)bad_frame, (unsigned int)handler_signature);
    return 1;
}
