#include <stdint.h>

#define UART_BASE           0x30000000u
#define UART_TX             (*(volatile uint32_t *)(UART_BASE + 0x04u))

#define PTE_V               (1u << 0)
#define PTE_R               (1u << 1)
#define PTE_W               (1u << 2)
#define PTE_X               (1u << 3)
#define PTE_A               (1u << 6)
#define PTE_D               (1u << 7)

#define SATP_MODE_SV32      (1u << 31)
#define MSTATUS_MPP_MASK    (3u << 11)
#define MSTATUS_MPP_S       (1u << 11)

#define VA_DATA             0x40000000u
#define VA_FUNC             0x50000000u

extern void s_entry(void);
extern void fn_a(void);
extern void fn_b(void);

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static uint32_t l1_data[1024] __attribute__((aligned(4096)));
static uint32_t l1_func[1024] __attribute__((aligned(4096)));
static volatile uint32_t data_a = 0x13572468u;
static volatile uint32_t data_b __attribute__((aligned(4096))) = 0x24681357u;
static volatile uint32_t test_fail;

extern void uart_puts_raw(const char *str);

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

static uint32_t func_pte(void (*fn)(void))
{
    return pte((uint32_t)fn, PTE_R | PTE_X);
}

static inline void sfence_vma(void)
{
    asm volatile("sfence.vma" ::: "memory");
}

static inline void csr_write_satp(uint32_t val)
{
    asm volatile("csrw satp, %0\nsfence.vma" :: "r"(val) : "memory");
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

static void setup_page_table(void)
{
    root_pt[0x10000000u >> 22u] = pte(0x10000000u, PTE_R | PTE_X);
    root_pt[VA_FUNC >> 22u] = table_pte(l1_func);
    l1_func[(VA_FUNC >> 12u) & 0x3ffu] = func_pte(fn_a);

    root_pt[0x20000000u >> 22u] = pte(0x20000000u, PTE_R | PTE_W);
    root_pt[UART_BASE >> 22u] = pte(UART_BASE, PTE_R | PTE_W);

    root_pt[VA_DATA >> 22u] = table_pte(l1_data);
    l1_data[(VA_DATA >> 12u) & 0x3ffu] = ptr_pte((void *)&data_a);
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
    setup_page_table();
    csr_write_satp(SATP_MODE_SV32 | ((uint32_t)root_pt >> 12u));
    enter_s_mode();

    while (1) {
        asm volatile("wfi");
    }
}

asm(
    ".section .text\n"
    ".globl uart_puts_raw\n"
    ".type uart_puts_raw,@function\n"
    "uart_puts_raw:\n"
    "    lbu t0, 0(a0)\n"
    "    beqz t0, 1f\n"
    "    li t1, 0x30000004\n"
    "0:\n"
    "    sw t0, 0(t1)\n"
    "    addi a0, a0, 1\n"
    "    lbu t0, 0(a0)\n"
    "    bnez t0, 0b\n"
    "1:\n"
    "    ret\n"
    "\n"
    ".align 12\n"
    ".globl fn_a\n"
    ".type fn_a,@function\n"
    "fn_a:\n"
    "    li a0, 0x11110001\n"
    "    ret\n"
    "\n"
    ".align 12\n"
    ".globl fn_b\n"
    ".type fn_b,@function\n"
    "fn_b:\n"
    "    li a0, 0x22220002\n"
    "    ret\n"
    "\n"
    ".align 2\n"
    ".globl s_entry\n"
    ".type s_entry,@function\n"
    "s_entry:\n"
    "    li t0, 0x40000000\n"
    "    lw t1, 0(t0)\n"
    "    li t2, 0x13572468\n"
    "    bne t1, t2, 9f\n"
    "    lw t1, 0(t0)\n"
    "    bne t1, t2, 9f\n"
    "    lw t1, 0(t0)\n"
    "    bne t1, t2, 9f\n"
    "    li t3, 0x50000000\n"
    "    jalr ra, 0(t3)\n"
    "    jalr ra, 0(t3)\n"
    "    li t2, 0x11110001\n"
    "    bne a0, t2, 9f\n"
    "    la t0, l1_data\n"
    "    li t1, 0x40000000\n"
    "    srli t1, t1, 12\n"
    "    andi t1, t1, 1023\n"
    "    slli t1, t1, 2\n"
    "    add t0, t0, t1\n"
    "    la t2, data_b\n"
    "    srli t2, t2, 12\n"
    "    slli t2, t2, 10\n"
    "    li t4, 0xc7\n"
    "    or t2, t2, t4\n"
    "    sw t2, 0(t0)\n"
    "    la t0, l1_func\n"
    "    li t1, 0x50000000\n"
    "    srli t1, t1, 12\n"
    "    andi t1, t1, 1023\n"
    "    slli t1, t1, 2\n"
    "    add t0, t0, t1\n"
    "    la t2, fn_b\n"
    "    srli t2, t2, 12\n"
    "    slli t2, t2, 10\n"
    "    li t4, 0xcb\n"
    "    or t2, t2, t4\n"
    "    sw t2, 0(t0)\n"
    "    sfence.vma\n"
    "    li t0, 0x40000000\n"
    "    lw t1, 0(t0)\n"
    "    li t2, 0x24681357\n"
    "    bne t1, t2, 9f\n"
    "    li t3, 0x50000000\n"
    "    jalr ra, 0(t3)\n"
    "    li t2, 0x22220002\n"
    "    bne a0, t2, 9f\n"
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
    "    li t0, 0x30000004\n"
    "    li t1, 0x10\n"
    "    sw t1, 0(t0)\n"
    "    wfi\n"
    "    j .\n"
    "\n"
    ".section .rodata\n"
    "pass_msg: .asciz \"tlb: PASS\\n\"\n"
    "fail_msg: .asciz \"tlb: FAIL\\n\"\n"
);
