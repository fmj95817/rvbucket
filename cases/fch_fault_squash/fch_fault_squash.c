#include <stdint.h>

#define ITCM_BASE        0x10000000u
#define DTCM_BASE        0x20000000u
#define UART_BASE        0x30000000u
#define UART_TX          (*(volatile uint32_t *)(UART_BASE + 0x04u))

#define PTE_V            (1u << 0)
#define PTE_R            (1u << 1)
#define PTE_W            (1u << 2)
#define PTE_X            (1u << 3)
#define PTE_A            (1u << 6)
#define PTE_D            (1u << 7)

#define SATP_MODE_SV32   (1u << 31)
#define MSTATUS_MPP_MASK (3u << 11)
#define MSTATUS_MPP_S    (1u << 11)
#define INST_PAGE_FAULT  12u

extern void m_trap_entry(void);
extern void s_trap_entry(void);
extern void s_mode_entry(void);
extern void ifetch_branch(void);
extern void ifetch_target(void);
extern void ifetch_wrong_path(void);

int test_finish(void);

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t code_pt[1024] __attribute__((aligned(4096)));
static volatile uint32_t target_seen;
static volatile uint32_t wrong_path_seen;
static volatile uint32_t fault_seen;
static volatile uint32_t bad_fault;

static inline void csr_write_mstatus(uint32_t val)
{
    asm volatile("csrw mstatus, %0" :: "r"(val) : "memory");
}

static inline uint32_t csr_read_mstatus(void)
{
    uint32_t val;
    asm volatile("csrr %0, mstatus" : "=r"(val));
    return val;
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

static uint32_t leaf_pte(uint32_t pa, uint32_t flags)
{
    return ((pa >> 12u) << 10u) | flags | PTE_V | PTE_A | PTE_D;
}

static uint32_t table_pte(const uint32_t *table)
{
    return (((uint32_t)table >> 12u) << 10u) | PTE_V;
}

static void setup_page_table(void)
{
    for (uint32_t i = 0; i < 1024; i++) {
        code_pt[i] = leaf_pte(ITCM_BASE + i * 4096u, PTE_R | PTE_X);
    }

    uint32_t target_vpn0 = ((uint32_t)ifetch_target >> 12u) & 0x3ffu;
    code_pt[target_vpn0] = 0;

    root_pt[ITCM_BASE >> 22u] = table_pte(code_pt);
    root_pt[DTCM_BASE >> 22u] = leaf_pte(DTCM_BASE, PTE_R | PTE_W);
    root_pt[UART_BASE >> 22u] = leaf_pte(UART_BASE, PTE_R | PTE_W);
}

static void map_target_page(void)
{
    uint32_t target = (uint32_t)ifetch_target;
    uint32_t vpn0 = (target >> 12u) & 0x3ffu;
    code_pt[vpn0] = leaf_pte(target & ~0xfffu, PTE_R | PTE_X);
    asm volatile("sfence.vma" ::: "memory");
}

uint32_t s_trap_handler(uint32_t scause, uint32_t sepc, uint32_t stval)
{
    uint32_t target = (uint32_t)ifetch_target;

    if (scause == INST_PAGE_FAULT && sepc == target && stval == target) {
        fault_seen++;
        map_target_page();
        return sepc;
    }

    bad_fault = 1;
    return (uint32_t)test_finish;
}

uint32_t m_trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval)
{
    (void)mcause;
    (void)mepc;
    (void)mtval;
    bad_fault = 1;
    return (uint32_t)test_finish;
}

static void uart_puts(const char *str)
{
    while (*str != '\0') {
        UART_TX = (uint8_t)*str++;
    }
}

int test_finish(void)
{
    if (target_seen == 1u && wrong_path_seen == 0u &&
        fault_seen == 1u && bad_fault == 0u) {
        uart_puts("ifetch_fault_squash: PASS\n");
        return 0;
    } else {
        uart_puts("ifetch_fault_squash: FAIL\n");
        return 1;
    }

    UART_TX = 0x10u;
    while (1) {
        asm volatile("wfi");
    }
    return 0;
}

static void enter_supervisor(void)
{
    setup_page_table();
    csr_write_mtvec((uintptr_t)m_trap_entry);
    csr_write_stvec((uintptr_t)s_trap_entry);
    asm volatile("csrs medeleg, %0" :: "r"(1u << INST_PAGE_FAULT));
    csr_write_satp(SATP_MODE_SV32 | ((uint32_t)root_pt >> 12u));

    uint32_t mstatus = csr_read_mstatus();
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    csr_write_mepc((uintptr_t)s_mode_entry);
    asm volatile("mret" ::: "memory");
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl m_trap_entry\n"
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
    "s_mode_entry:\n"
    "    li t0, 1\n"
    "    j ifetch_branch\n"
    "\n"
    ".section .text.ifetch_fault,\"ax\"\n"
    ".balign 4096\n"
    ".space 4092\n"
    ".globl ifetch_branch\n"
    "ifetch_branch:\n"
    "    bne t0, zero, ifetch_target\n"
    ".globl ifetch_wrong_path\n"
    "ifetch_wrong_path:\n"
    "    la t1, wrong_path_seen\n"
    "    li t2, 1\n"
    "    sw t2, 0(t1)\n"
    "    j test_finish\n"
    ".balign 32\n"
    ".globl ifetch_target\n"
    "ifetch_target:\n"
    "    la t1, target_seen\n"
    "    li t2, 1\n"
    "    sw t2, 0(t1)\n"
    "    j test_finish\n"
);

int main(void)
{
    target_seen = 0;
    wrong_path_seen = 0;
    fault_seen = 0;
    bad_fault = 0;
    enter_supervisor();
    return test_finish();
}
