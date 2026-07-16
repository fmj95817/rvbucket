#include <stdint.h>
#include <stdio.h>

#define DDR_BASE           0x40000000u
#define PTE_V              (1u << 0)
#define PTE_R              (1u << 1)
#define PTE_W              (1u << 2)
#define PTE_X              (1u << 3)
#define PTE_A              (1u << 6)
#define PTE_D              (1u << 7)
#define SATP_MODE_SV32     (1u << 31)
#define MSTATUS_MPP_MASK   (3u << 11)
#define MSTATUS_MPP_M      (3u << 11)
#define MSTATUS_MPP_S      (1u << 11)
#define MCAUSE_ECALL_S     9u

extern void s_satp_entry(void);
extern void s_satp_after(void);
extern void m_mode_resume(void);

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static volatile uint32_t good_path;
static volatile uint32_t stale_path;
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

static uint32_t encode_lui(uint32_t rd, uintptr_t value)
{
    return (((value + 0x800u) >> 12u) << 12u) | (rd << 7u) | 0x37u;
}

static uint32_t encode_addi(uint32_t rd, uint32_t rs1, uintptr_t value)
{
    uint32_t imm = value & 0xfffu;
    return (imm << 20u) | (rs1 << 15u) | (rd << 7u) | 0x13u;
}

static void setup_redirect_code(void)
{
    uintptr_t next_pc = (uintptr_t)s_satp_after;
    volatile uint32_t *code = (volatile uint32_t *)(DDR_BASE +
        (next_pc & 0x003fffffu));
    uintptr_t flag_addr = (uintptr_t)&good_path;

    /* li t0,&good_path; li t1,1; sw t1,0(t0); ecall */
    code[0] = encode_lui(5u, flag_addr);
    code[1] = encode_addi(5u, 5u, flag_addr);
    code[2] = 0x00100313u;
    code[3] = 0x0062a023u;
    code[4] = 0x00000073u;
    asm volatile(".word 0x0000100f" ::: "memory");
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

__attribute__((noinline)) static void enter_supervisor(void)
{
    uint32_t satp = SATP_MODE_SV32 | ((uintptr_t)root_pt >> 12u);
    uint32_t mstatus = csr_read_mstatus();

    m_resume_pc = (uintptr_t)m_mode_resume;
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    asm volatile("csrw mepc, %0" :: "r"(s_satp_entry) : "memory");
    asm volatile(
        "mv a0, %0\n"
        "mret"
        :: "r"(satp) : "a0", "memory");
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl s_satp_entry\n"
    ".type s_satp_entry,@function\n"
    "s_satp_entry:\n"
    "    csrw satp, a0\n"
    ".globl s_satp_after\n"
    "s_satp_after:\n"
    "    la t0, stale_path\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    ecall\n"
    "\n"
    ".align 2\n"
    ".globl m_mode_resume\n"
    ".type m_mode_resume,@function\n"
    "m_mode_resume:\n"
    "    ret\n"
);

int main(void)
{
    good_path = 0;
    stale_path = 0;
    bad_mcause = 0;

    root_pt[0x10000000u >> 22u] =
        superpage_pte(DDR_BASE, PTE_R | PTE_X);
    root_pt[0x20000000u >> 22u] =
        superpage_pte(0x20000000u, PTE_R | PTE_W);
    setup_redirect_code();

    enter_supervisor();

    if (good_path == 1u && stale_path == 0u && bad_mcause == 0u) {
        printf("satp_switch: PASS\n");
        return 0;
    }

    printf("satp_switch: FAIL good=%u stale=%u mcause=0x%08x\n",
        (unsigned int)good_path, (unsigned int)stale_path,
        (unsigned int)bad_mcause);
    return 1;
}
