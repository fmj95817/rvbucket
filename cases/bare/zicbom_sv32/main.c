#include <stdint.h>

#define DDR_BASE         0x40000000u
#define DDR_ALIAS_BASE   0x50000000u
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

static uint32_t root_pt[1024] __attribute__((aligned(4096)));
static volatile uint32_t test_line[16] __attribute__((aligned(64)));

static inline void cbo_inval(const volatile void *addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 0"
        : : "r" (addr) : "memory");
}

static inline void cbo_clean(const volatile void *addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 1"
        : : "r" (addr) : "memory");
}

static inline void cbo_flush(const volatile void *addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 2"
        : : "r" (addr) : "memory");
}

static inline void memory_fence(void)
{
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

static uint32_t superpage_pte(uint32_t pa, uint32_t flags)
{
    return ((pa >> 12u) << 10u) | flags | PTE_V | PTE_A | PTE_D;
}

static void uart_puts_raw(const char *str)
{
    while (*str != '\0') {
        UART_TX = (uint8_t)*str;
        str++;
    }
}

static void sim_finish(uint32_t exit_code) __attribute__((noreturn));

static void sim_finish(uint32_t exit_code)
{
    __asm__ volatile (
        "mv t0, %0\n"
        "li t1, 0x30000004\n"
        "li t2, 0x10\n"
        "sw t2, 0(t1)\n"
        "1:\n"
        "wfi\n"
        "j 1b\n"
        :
        : "r" (exit_code)
        : "t0", "t1", "t2", "memory");
    __builtin_unreachable();
}

static void fill_line(volatile uint32_t *line, uint32_t seed)
{
    for (uint32_t i = 0; i < 16; i++)
        line[i] = seed ^ (i * 0x01010101u);
}

static int check_line(const volatile uint32_t *line, uint32_t seed)
{
    for (uint32_t i = 0; i < 16; i++) {
        if (line[i] != (seed ^ (i * 0x01010101u)))
            return -1;
    }
    return 0;
}

static void s_mode_entry(void) __attribute__((noreturn));

static void s_mode_entry(void)
{
    uintptr_t offset = (uintptr_t)test_line - DDR_BASE;
    volatile uint32_t *alias =
        (volatile uint32_t *)(DDR_ALIAS_BASE + offset);

    fill_line(alias, 0x22220000u);
    memory_fence();
    cbo_clean(alias);
    memory_fence();
    cbo_inval(test_line);
    memory_fence();
    if (check_line(test_line, 0x22220000u) != 0) {
        uart_puts_raw("zicbom_sv32: clean failed\n");
        sim_finish(1);
    }

    fill_line(alias, 0x33330000u);
    memory_fence();
    cbo_inval(alias);
    memory_fence();
    if (check_line(test_line, 0x22220000u) != 0) {
        uart_puts_raw("zicbom_sv32: inval failed\n");
        sim_finish(2);
    }

    fill_line(alias, 0x44440000u);
    memory_fence();
    cbo_flush(alias);
    memory_fence();
    if (check_line(test_line, 0x44440000u) != 0) {
        uart_puts_raw("zicbom_sv32: flush failed\n");
        sim_finish(3);
    }

    uart_puts_raw("zicbom_sv32: PASS\n");
    sim_finish(0);
}

int main(void)
{
    fill_line(test_line, 0x11110000u);
    memory_fence();
    cbo_flush(test_line);
    memory_fence();

    root_pt[DDR_BASE >> 22u] =
        superpage_pte(DDR_BASE, PTE_R | PTE_W | PTE_X);
    root_pt[DDR_ALIAS_BASE >> 22u] =
        superpage_pte(DDR_BASE, PTE_R | PTE_W);
    root_pt[UART_BASE >> 22u] =
        superpage_pte(UART_BASE, PTE_R | PTE_W);

    uint32_t satp = SATP_MODE_SV32 | ((uint32_t)root_pt >> 12u);
    __asm__ volatile ("csrw satp, %0\nsfence.vma" : : "r" (satp) : "memory");

    uint32_t mstatus;
    __asm__ volatile ("csrr %0, mstatus" : "=r" (mstatus));
    mstatus = (mstatus & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S;
    __asm__ volatile ("csrw mstatus, %0" : : "r" (mstatus) : "memory");
    __asm__ volatile ("csrw mepc, %0" : : "r" (s_mode_entry) : "memory");
    __asm__ volatile ("mret" : : : "memory");

    __builtin_unreachable();
}
