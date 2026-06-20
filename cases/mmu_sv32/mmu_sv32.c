#include <stdint.h>

#define DTCM_BASE           0x20000000u
#define UART_BASE           0x30000000u
#define HIGH_DTCM_BASE      0x40000000u
#define UART_TX             (*(volatile uint32_t *)(UART_BASE + 0x04u))

#define PTE_V               (1u << 0)
#define PTE_R               (1u << 1)
#define PTE_W               (1u << 2)
#define PTE_X               (1u << 3)
#define PTE_A               (1u << 6)
#define PTE_D               (1u << 7)

#define SATP_MODE_SV32      (1u << 31)
#define MSTATUS_MPP_MASK    (3u << 11)
#define MSTATUS_MPP_M       (3u << 11)
#define MSTATUS_MPP_S       (1u << 11)
#define MEDELEG_PAGE_FAULTS ((1u << 12) | (1u << 13) | (1u << 15))
#define MCAUSE_LOAD_PAGE    13u

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
void s_finish(void) __attribute__((noreturn));

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static volatile uint32_t target;
static volatile uint32_t s_started;
static volatile uint32_t va_rw_ok;
static volatile uint32_t load_fault_seen;
static volatile uint32_t bad_scause;
static volatile uint32_t bad_mcause;

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

static inline void csr_write_satp(uint32_t val)
{
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(val) : "memory");
}

static inline void csr_set_medeleg(uint32_t val)
{
    asm volatile("csrs medeleg, %0" :: "r"(val) : "memory");
}

static uint32_t superpage_pte(uint32_t pa, uint32_t flags)
{
    return ((pa >> 12u) << 10u) | flags | PTE_V | PTE_A | PTE_D;
}

static void setup_page_table(void)
{
    root_pt[0x10000000u >> 22u] = superpage_pte(0x10000000u, PTE_R | PTE_X);
    root_pt[DTCM_BASE >> 22u] = superpage_pte(DTCM_BASE, PTE_R | PTE_W);
    root_pt[UART_BASE >> 22u] = superpage_pte(UART_BASE, PTE_R | PTE_W);
    root_pt[HIGH_DTCM_BASE >> 22u] = superpage_pte(DTCM_BASE, PTE_R | PTE_W);
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval)
{
    (void)sepc;

    if (scause == MCAUSE_LOAD_PAGE && stval == 0x60000000u) {
        load_fault_seen = 1;
        return (uint32_t)s_finish;
    }

    bad_scause = scause;
    return (uint32_t)s_finish;
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mepc;
    (void)mtval;

    bad_mcause = mcause;
    return mepc + 4u;
}

static void uart_puts_raw(const char *str)
{
    while (*str != '\0') {
        UART_TX = (uint8_t)*str;
        str++;
    }
}

void s_finish(void)
{
    if (s_started == 1u &&
        va_rw_ok == 1u &&
        target == 0x5a5a0001u &&
        load_fault_seen == 1u &&
        bad_scause == 0u &&
        bad_mcause == 0u) {
        uart_puts_raw("mmu_sv32: PASS\n");
    } else {
        uart_puts_raw("mmu_sv32: FAIL\n");
    }

    UART_TX = 0x10u;
    while (1) {
        asm volatile("wfi");
    }
}

__attribute__((noinline)) static void enter_supervisor_with_mmu(void)
{
    setup_page_table();

    csr_write_mtvec((uintptr_t)m_trap_entry);
    csr_write_stvec((uintptr_t)s_trap_entry);
    csr_set_medeleg(MEDELEG_PAGE_FAULTS);
    csr_write_satp(SATP_MODE_SV32 | ((uint32_t)root_pt >> 12u));

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
    "    sw a0,  4(sp)\n"
    "    sw a1,  8(sp)\n"
    "    sw a2, 12(sp)\n"
    "    csrr a0, mcause\n"
    "    csrr a1, mepc\n"
    "    csrr a2, mtval\n"
    "    call m_trap_handler\n"
    "    csrw mepc, a0\n"
    "    lw ra,  0(sp)\n"
    "    lw a0,  4(sp)\n"
    "    lw a1,  8(sp)\n"
    "    lw a2, 12(sp)\n"
    "    addi sp, sp, 32\n"
    "    mret\n"
    "\n"
    ".align 2\n"
    ".globl s_trap_entry\n"
    ".type s_trap_entry,@function\n"
    "s_trap_entry:\n"
    "    addi sp, sp, -32\n"
    "    sw ra,  0(sp)\n"
    "    sw a0,  4(sp)\n"
    "    sw a1,  8(sp)\n"
    "    sw a2, 12(sp)\n"
    "    csrr a0, scause\n"
    "    csrr a1, sepc\n"
    "    csrr a2, stval\n"
    "    call s_trap_handler\n"
    "    csrw sepc, a0\n"
    "    lw ra,  0(sp)\n"
    "    lw a0,  4(sp)\n"
    "    lw a1,  8(sp)\n"
    "    lw a2, 12(sp)\n"
    "    addi sp, sp, 32\n"
    "    sret\n"
    "\n"
    ".align 2\n"
    ".globl s_mode_entry\n"
    ".type s_mode_entry,@function\n"
    "s_mode_entry:\n"
    "    la t0, s_started\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "    la t0, target\n"
    "    li t1, 0x20000000\n"
    "    sub t0, t0, t1\n"
    "    li t1, 0x40000000\n"
    "    add t0, t0, t1\n"
    "    li t1, 0x5a5a0001\n"
    "    sw t1, 0(t0)\n"
    "    lw t2, 0(t0)\n"
    "    bne t1, t2, 1f\n"
    "    la t0, va_rw_ok\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "1:\n"
    "    li t0, 0x60000000\n"
    "    lw t1, 0(t0)\n"
    "    j .\n"
);

int main(void)
{
    target = 0;
    s_started = 0;
    va_rw_ok = 0;
    load_fault_seen = 0;
    bad_scause = 0;
    bad_mcause = 0;

    enter_supervisor_with_mmu();

    while (1) {
        asm volatile("wfi");
    }
}
