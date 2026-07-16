#include <stdint.h>
#include <stdio.h>

#define DDR_ENTRY           0x40400000u
#define DDR_FLAG_ADDR       0x40401000u
#define MSTATUS_MPP_MASK    (3u << 11)
#define MSTATUS_MPP_M       (3u << 11)
#define MSTATUS_MPP_S       (1u << 11)
#define MCAUSE_ECALL_S      9u

extern void m_mode_resume(void);

static volatile uint32_t m_returned;
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

static uint32_t encode_lui(uint32_t rd, uintptr_t value)
{
    return (((value + 0x800u) >> 12u) << 12u) | (rd << 7u) | 0x37u;
}

static uint32_t encode_addi(uint32_t rd, uint32_t rs1, uintptr_t value)
{
    uint32_t imm = value & 0xfffu;
    return (imm << 20u) | (rs1 << 15u) | (rd << 7u) | 0x13u;
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

__attribute__((noinline)) static void enter_ddr_supervisor(void)
{
    volatile uint32_t *ddr = (volatile uint32_t *)DDR_ENTRY;
    uint32_t mstatus;

    /* li t0,DDR_FLAG_ADDR; li t1,1; sw t1,0(t0); ecall */
    ddr[0] = encode_lui(5u, DDR_FLAG_ADDR);
    ddr[1] = encode_addi(5u, 5u, DDR_FLAG_ADDR);
    ddr[2] = 0x00100313u;
    ddr[3] = 0x0062a023u;
    ddr[4] = 0x00000073u;
    asm volatile(".word 0x0000100f" ::: "memory");

    m_resume_pc = (uintptr_t)m_mode_resume;
    mstatus = csr_read_mstatus();
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    asm volatile("csrw mepc, %0" :: "r"(DDR_ENTRY) : "memory");
    asm volatile("mret" ::: "memory");
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_mode_resume\n"
    ".type m_mode_resume,@function\n"
    "m_mode_resume:\n"
    "    ret\n"
);

int main(void)
{
    volatile uint32_t *flag = (volatile uint32_t *)DDR_FLAG_ADDR;

    *flag = 0;
    m_returned = 0;
    bad_mcause = 0;

    enter_ddr_supervisor();
    m_returned = 1;

    if (*flag == 1u && m_returned == 1u && bad_mcause == 0u) {
        printf("priv_ddr: PASS\n");
        return 0;
    }

    printf("priv_ddr: FAIL flag=%u returned=%u mcause=0x%08x\n",
        (unsigned int)*flag, (unsigned int)m_returned,
        (unsigned int)bad_mcause);
    return 1;
}
