#include <stdint.h>

#define DTCM_BASE           0x20000000u
#define UART_BASE           0x30000000u
#define UART_TX             (*(volatile uint32_t *)(UART_BASE + 0x04u))

#define VA_TEST             0x40000000u

#define PTE_V               (1u << 0)
#define PTE_R               (1u << 1)
#define PTE_W               (1u << 2)
#define PTE_X               (1u << 3)
#define PTE_A               (1u << 6)
#define PTE_D               (1u << 7)

#define SATP_MODE_SV32      (1u << 31)
#define MSTATUS_MPP_MASK    (3u << 11)
#define MSTATUS_MPP_S       (1u << 11)

extern void s_entry(void);

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t l1_test[1024] __attribute__((aligned(4096)));
static volatile uint8_t page_a[4096] __attribute__((aligned(4096)));
static volatile uint8_t page_gap[4096] __attribute__((aligned(4096)));
static volatile uint8_t page_b[4096] __attribute__((aligned(4096)));
static volatile uint32_t test_fail;

static uint32_t pte(uint32_t pa, uint32_t flags)
{
    return ((pa >> 12u) << 10u) | flags | PTE_V | PTE_A | PTE_D;
}

static uint32_t table_pte(void *ptr)
{
    return ((uint32_t)ptr >> 12u) << 10u | PTE_V;
}

static uint32_t ptr_pte(void *ptr)
{
    return pte((uint32_t)ptr, PTE_R | PTE_W);
}

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

static inline void csr_write_satp(uint32_t val)
{
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(val) : "memory");
}

__attribute__((used, noinline, section(".text"))) static void uart_puts_raw(
    const char *str)
{
    while (*str != '\0') {
        UART_TX = (uint8_t)*str;
        str++;
    }
}

static void setup_page_table(void)
{
    root_pt[0x10000000u >> 22u] = pte(0x10000000u, PTE_R | PTE_X);
    root_pt[DTCM_BASE >> 22u] = pte(DTCM_BASE, PTE_R | PTE_W);
    root_pt[UART_BASE >> 22u] = pte(UART_BASE, PTE_R | PTE_W);
    root_pt[VA_TEST >> 22u] = table_pte(l1_test);
    l1_test[(VA_TEST >> 12u) & 0x3ffu] = ptr_pte((void *)page_a);
    l1_test[((VA_TEST >> 12u) & 0x3ffu) + 1u] =
        ptr_pte((void *)page_b);
}

static void enter_s_mode(void)
{
    uint32_t mstatus = csr_read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    csr_write_mstatus(mstatus);
    csr_write_mepc((uintptr_t)s_entry);
    asm volatile("mret" ::: "memory");
}

int main(void)
{
    page_a[4094] = 0x34u;
    page_a[4095] = 0x12u;
    page_gap[0] = 0xeeu;
    page_gap[1] = 0xeeu;
    page_b[0] = 0x78u;
    page_b[1] = 0x56u;
    test_fail = 0;

    setup_page_table();
    csr_write_satp(SATP_MODE_SV32 | ((uint32_t)root_pt >> 12u));
    enter_s_mode();

    while (1) {
        asm volatile("wfi");
    }
}

asm(
    ".section .text\n"
    ".align 2\n"
    ".globl s_entry\n"
    ".type s_entry,@function\n"
    "s_entry:\n"
    "    li t0, 0x40000fff\n"
    "    lhu t1, 0(t0)\n"
    "    li t2, 0x7812\n"
    "    bne t1, t2, 9f\n"
    "    li t0, 0x40000ffe\n"
    "    lw t1, 0(t0)\n"
    "    li t2, 0x56781234\n"
    "    bne t1, t2, 9f\n"
    "    li t0, 0x40000fff\n"
    "    li t1, 0xa1b2\n"
    "    sh t1, 0(t0)\n"
    "    la t0, page_a\n"
    "    li t3, 4095\n"
    "    add t0, t0, t3\n"
    "    lbu t1, 0(t0)\n"
    "    li t2, 0xb2\n"
    "    bne t1, t2, 9f\n"
    "    la t0, page_b\n"
    "    lbu t1, 0(t0)\n"
    "    li t2, 0xa1\n"
    "    bne t1, t2, 9f\n"
    "    li t0, 0x40000ffe\n"
    "    li t1, 0xc3d4e5f6\n"
    "    sw t1, 0(t0)\n"
    "    la t0, page_a\n"
    "    li t3, 4094\n"
    "    add t0, t0, t3\n"
    "    lbu t1, 0(t0)\n"
    "    li t2, 0xf6\n"
    "    bne t1, t2, 9f\n"
    "    lbu t1, 1(t0)\n"
    "    li t2, 0xe5\n"
    "    bne t1, t2, 9f\n"
    "    la t0, page_b\n"
    "    lbu t1, 0(t0)\n"
    "    li t2, 0xd4\n"
    "    bne t1, t2, 9f\n"
    "    lbu t1, 1(t0)\n"
    "    li t2, 0xc3\n"
    "    bne t1, t2, 9f\n"
    "    j 10f\n"
    "9:\n"
    "    la t0, test_fail\n"
    "    li t1, 1\n"
    "    sw t1, 0(t0)\n"
    "10:\n"
    "    la t0, test_fail\n"
    "    lw t1, 0(t0)\n"
    "    bnez t1, 11f\n"
    "    la a0, pass_msg\n"
    "    j 12f\n"
    "11:\n"
    "    la a0, fail_msg\n"
    "12:\n"
    "    call uart_puts_raw\n"
    "    li t2, 0x30000004\n"
    "    li t1, 0x10\n"
    "    sw t1, 0(t2)\n"
    "    wfi\n"
    "    j .\n"
    "\n"
    ".section .rodata\n"
    "pass_msg: .asciz \"cross_page: PASS\\n\"\n"
    "fail_msg: .asciz \"cross_page: FAIL\\n\"\n"
);
