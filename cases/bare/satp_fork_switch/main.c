#include <stdint.h>
#include <stdio.h>

#define DDR_BASE             0x40000000u
#define USER_CODE_VA         0x0014a000u
#define USER_DATA_VA         0x0014b000u
#define PTE_V                (1u << 0)
#define PTE_R                (1u << 1)
#define PTE_W                (1u << 2)
#define PTE_X                (1u << 3)
#define PTE_U                (1u << 4)
#define PTE_A                (1u << 6)
#define PTE_D                (1u << 7)
#define SATP_MODE_SV32       (1u << 31)
#define MSTATUS_MPP_MASK     (3u << 11)
#define MSTATUS_MPP_M        (3u << 11)
#define MSTATUS_MPP_S        (1u << 11)
#define SSTATUS_SPP          (1u << 8)
#define MIE_STIE             (1u << 5)
#define MIDELEG_S_TIMER      (1u << 5)
#define MCAUSE_INTERRUPT     0x80000000u
#define SCAUSE_S_TIMER       (MCAUSE_INTERRUPT | 5u)
#define MCAUSE_ECALL_U       8u
#define MCAUSE_ECALL_S       9u
#define ROOT_NUM             16u
#define SWITCH_NUM           16u
#define S_TIMER_INTERVAL     1u
#define S_TRAP_TO_M          0xffffffffu

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void s_mode_finish(void);
extern void m_mode_resume(void);

static uint32_t root_pt[ROOT_NUM][1024] __attribute__((aligned(4096)));
static uint32_t user_l0_pt[ROOT_NUM][1024] __attribute__((aligned(4096)));
static uint32_t user_code_page[1024] __attribute__((aligned(4096)));
static uint32_t user_data_page[ROOT_NUM][1024]
    __attribute__((aligned(4096)));
static volatile uint32_t switch_count;
static volatile uint32_t s_timer_irq_count;
static volatile uint32_t bad_scause;
static volatile uint32_t bad_mcause;
static volatile uint32_t bad_sstatus;
static uintptr_t m_resume_pc;
uintptr_t m_resume_ra;

static inline uint32_t csr_read_mstatus(void)
{
    uint32_t value;
    asm volatile("csrr %0, mstatus" : "=r"(value));
    return value;
}

static inline uint32_t csr_read_sstatus(void)
{
    uint32_t value;
    asm volatile("csrr %0, sstatus" : "=r"(value));
    return value;
}

static inline void csr_write_mstatus(uint32_t value)
{
    asm volatile("csrw mstatus, %0" :: "r"(value) : "memory");
}

static inline void csr_set_mideleg(uint32_t value)
{
    asm volatile("csrs mideleg, %0" :: "r"(value) : "memory");
}

static inline void csr_set_mie(uint32_t value)
{
    asm volatile("csrs mie, %0" :: "r"(value) : "memory");
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

static void stimecmp_write(uint64_t value)
{
    asm volatile("csrw 0x15d, %0" :: "r"((uint32_t)(value >> 32)) : "memory");
    asm volatile("csrw 0x14d, %0" :: "r"((uint32_t)value) : "memory");
}

static uint32_t pte_from_pa(uintptr_t pa, uint32_t flags)
{
    return ((uint32_t)(pa >> 12u) << 10u) | flags | PTE_V;
}

static void setup_page_tables(void)
{
    for (uint32_t i = 0; i < ROOT_NUM; i++) {
        root_pt[i][DDR_BASE >> 22u] = pte_from_pa(DDR_BASE,
            PTE_R | PTE_W | PTE_X | PTE_A | PTE_D);
        root_pt[i][USER_CODE_VA >> 22u] =
            pte_from_pa((uintptr_t)user_l0_pt[i], 0u);
        user_l0_pt[i][(USER_CODE_VA >> 12u) & 0x3ffu] =
            pte_from_pa((uintptr_t)user_code_page,
                PTE_R | PTE_X | PTE_U | PTE_A | PTE_D);
        user_l0_pt[i][(USER_DATA_VA >> 12u) & 0x3ffu] =
            pte_from_pa((uintptr_t)user_data_page[i],
                PTE_R | PTE_W | PTE_U | PTE_A | PTE_D);
    }

    user_code_page[0] = 0x0014b2b7u; /* lui t0,0x14b */
    user_code_page[1] = 0x0002a303u; /* lw t1,0(t0) */
    user_code_page[2] = 0x00130313u; /* addi t1,t1,1 */
    user_code_page[3] = 0x0062a023u; /* sw t1,0(t0) */
    user_code_page[4] = 0x00c0006fu; /* j load_ra_and_return */
    user_code_page[5] = 0x00000073u; /* ecall */
    user_code_page[6] = 0xfe9ff06fu; /* j USER_CODE_VA */
    user_code_page[7] = 0x0042a083u; /* lw ra,4(t0) */
    user_code_page[8] = 0x00008067u; /* ret */
    asm volatile(".word 0x0000100f" ::: "memory");
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval)
{
    (void)stval;

    if (scause == SCAUSE_S_TIMER) {
        if ((csr_read_sstatus() & SSTATUS_SPP) != 0u) {
            bad_sstatus++;
            return S_TRAP_TO_M;
        }
        s_timer_irq_count++;
        stimecmp_write(time_read() + S_TIMER_INTERVAL);
        return sepc;
    }

    if (scause != MCAUSE_ECALL_U ||
        (csr_read_sstatus() & SSTATUS_SPP) != 0u) {
        if ((csr_read_sstatus() & SSTATUS_SPP) != 0u) {
            bad_sstatus++;
        }
        bad_scause = scause;
        return S_TRAP_TO_M;
    }

    switch_count++;
    if (switch_count >= SWITCH_NUM) {
        return S_TRAP_TO_M;
    }

    uint32_t next_root = switch_count & (ROOT_NUM - 1u);
    uint32_t vpn1 = USER_CODE_VA >> 22u;
    uint32_t code_vpn0 = (USER_CODE_VA >> 12u) & 0x3ffu;
    uint32_t data_vpn0 = (USER_DATA_VA >> 12u) & 0x3ffu;

    /* Rebuild the inactive address space as fork would before scheduling it. */
    root_pt[next_root][vpn1] = 0u;
    user_l0_pt[next_root][code_vpn0] = 0u;
    user_l0_pt[next_root][data_vpn0] = 0u;
    asm volatile("fence rw, rw" ::: "memory");
    user_l0_pt[next_root][code_vpn0] =
        pte_from_pa((uintptr_t)user_code_page,
            PTE_R | PTE_X | PTE_U | PTE_A | PTE_D);
    user_l0_pt[next_root][data_vpn0] =
        pte_from_pa((uintptr_t)user_data_page[next_root],
            PTE_R | PTE_W | PTE_U | PTE_A | PTE_D);
    root_pt[next_root][vpn1] =
        pte_from_pa((uintptr_t)user_l0_pt[next_root], 0u);
    asm volatile("fence rw, rw" ::: "memory");

    uint32_t satp = SATP_MODE_SV32 |
        ((uint32_t)(uintptr_t)root_pt[next_root] >> 12u);
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(satp) : "memory");
    return sepc + 4u;
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mtval;

    if (mcause == MCAUSE_ECALL_S) {
        uint32_t mstatus = csr_read_mstatus();
        mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_M;
        csr_write_mstatus(mstatus);
        return (uint32_t)m_resume_pc;
    }

    bad_mcause = mcause;
    return mepc;
}

__attribute__((noinline)) static void enter_supervisor(void)
{
    uint32_t mstatus = csr_read_mstatus();
    uint32_t satp = SATP_MODE_SV32 |
        ((uint32_t)(uintptr_t)root_pt[0] >> 12u);
    uintptr_t resume_ra;

    m_resume_pc = (uintptr_t)m_mode_resume;
    asm volatile("mv %0, ra" : "=r"(resume_ra));
    m_resume_ra = resume_ra;
    asm volatile("csrw mtvec, %0" :: "r"(m_trap_entry) : "memory");
    asm volatile("csrw stvec, %0" :: "r"(s_trap_entry) : "memory");
    asm volatile("csrs medeleg, %0" :: "r"(1u << MCAUSE_ECALL_U) : "memory");
    csr_set_mideleg(MIDELEG_S_TIMER);
    csr_set_mie(MIE_STIE);
    stimecmp_write(time_read() + S_TIMER_INTERVAL);
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(satp) : "memory");
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    asm volatile("csrw mepc, %0" :: "r"(s_mode_entry) : "memory");
    asm volatile("mret" ::: "memory");
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_trap_entry\n"
    "m_trap_entry:\n"
    "    addi sp, sp, -32\n"
    "    sw ra, 0(sp)\n"
    "    sw a0, 4(sp)\n"
    "    sw a1, 8(sp)\n"
    "    sw a2, 12(sp)\n"
    "    csrr a0, mcause\n"
    "    csrr a1, mepc\n"
    "    csrr a2, mtval\n"
    "    call m_trap_handler\n"
    "    csrw mepc, a0\n"
    "    lw ra, 0(sp)\n"
    "    lw a0, 4(sp)\n"
    "    lw a1, 8(sp)\n"
    "    lw a2, 12(sp)\n"
    "    addi sp, sp, 32\n"
    "    mret\n"
    ".globl m_mode_resume\n"
    "m_mode_resume:\n"
    "    la t0, m_resume_ra\n"
    "    lw ra, 0(t0)\n"
    "    ret\n"
    ".align 2\n"
    ".globl s_trap_entry\n"
    "s_trap_entry:\n"
    "    addi sp, sp, -144\n"
    "    sw ra,   4(sp)\n"
    "    sw t0,  20(sp)\n"
    "    sw t1,  24(sp)\n"
    "    sw t2,  28(sp)\n"
    "    sw a0,  40(sp)\n"
    "    sw a1,  44(sp)\n"
    "    sw a2,  48(sp)\n"
    "    sw a3,  52(sp)\n"
    "    sw a4,  56(sp)\n"
    "    sw a5,  60(sp)\n"
    "    csrr t0, sepc\n"
    "    sw t0,   0(sp)\n"
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    csrr a2, stval\n"
    "    call s_trap_handler\n"
    "    mv t0, a0\n"
    "    li t1, -1\n"
    "    beq t0, t1, 1f\n"
    "    sw t0,   0(sp)\n"
    "    lw a2,   0(sp)\n"
    "    sc.w zero, a2, (sp)\n"
    "    csrw sepc, a2\n"
    "    lw ra,   4(sp)\n"
    "    lw t0,  20(sp)\n"
    "    lw t1,  24(sp)\n"
    "    lw t2,  28(sp)\n"
    "    lw a0,  40(sp)\n"
    "    lw a1,  44(sp)\n"
    "    lw a2,  48(sp)\n"
    "    lw a3,  52(sp)\n"
    "    lw a4,  56(sp)\n"
    "    lw a5,  60(sp)\n"
    "    addi sp, sp, 144\n"
    "    sret\n"
    "1:\n"
    "    lw ra,   4(sp)\n"
    "    lw t0,  20(sp)\n"
    "    lw t1,  24(sp)\n"
    "    lw t2,  28(sp)\n"
    "    lw a0,  40(sp)\n"
    "    lw a1,  44(sp)\n"
    "    lw a2,  48(sp)\n"
    "    lw a3,  52(sp)\n"
    "    lw a4,  56(sp)\n"
    "    lw a5,  60(sp)\n"
    "    addi sp, sp, 144\n"
    "    li t0, 0x100\n"
    "    csrs sstatus, t0\n"
    "    la t0, s_mode_finish\n"
    "    csrw sepc, t0\n"
    "    sret\n"
    ".globl s_mode_entry\n"
    "s_mode_entry:\n"
    "    li t0, 0x100\n"
    "    csrc sstatus, t0\n"
    "    li t0, 0x0014a000\n"
    "    csrw sepc, t0\n"
    "    sret\n"
    ".globl s_mode_finish\n"
    "s_mode_finish:\n"
    "    ecall\n"
    "2:\n"
    "    j 2b\n"
);

int main(void)
{
    switch_count = 0;
    s_timer_irq_count = 0;
    bad_scause = 0;
    bad_mcause = 0;
    bad_sstatus = 0;
    for (uint32_t i = 0; i < ROOT_NUM; i++) {
        user_data_page[i][0] = 0;
        user_data_page[i][1] = USER_CODE_VA + 5u * sizeof(uint32_t);
    }
    setup_page_tables();

    enter_supervisor();

    uint32_t data_sum = 0;
    uint32_t data_bad = 0;
    for (uint32_t i = 0; i < ROOT_NUM; i++) {
        data_sum += user_data_page[i][0];
        if (user_data_page[i][0] != SWITCH_NUM / ROOT_NUM) {
            data_bad++;
        }
    }

    if (switch_count == SWITCH_NUM && s_timer_irq_count != 0u &&
        bad_scause == 0u && bad_mcause == 0u && bad_sstatus == 0u &&
        data_sum == SWITCH_NUM && data_bad == 0u) {
        printf("satp_fork_switch: PASS switches=%u s_timer=%u\n",
            (unsigned int)switch_count, (unsigned int)s_timer_irq_count);
        return 0;
    }

    printf("satp_fork_switch: FAIL switches=%u s_timer=%u data_sum=%u "
        "data_bad=%u scause=0x%08x mcause=0x%08x bad_status=%u\n",
        (unsigned int)switch_count,
        (unsigned int)s_timer_irq_count,
        (unsigned int)data_sum,
        (unsigned int)data_bad,
        (unsigned int)bad_scause,
        (unsigned int)bad_mcause,
        (unsigned int)bad_sstatus);
    return 1;
}
