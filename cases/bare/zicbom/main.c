#include <stdint.h>
#include <stdio.h>

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

int main(void)
{
    const uint32_t committed_a = 0x12345678u;
    const uint32_t discarded_b = 0xdeadbeefu;
    const uint32_t committed_c = 0xa55a0ff0u;

    printf("zicbom: start\n");

    test_line[0] = committed_a;
    memory_fence();
    cbo_flush(test_line);
    memory_fence();
    if (test_line[0] != committed_a) {
        printf("zicbom: flush failed\n");
        return 1;
    }

    test_line[0] = discarded_b;
    memory_fence();
    cbo_inval(test_line);
    memory_fence();
    if (test_line[0] != committed_a) {
        printf("zicbom: inval wrote back dirty data\n");
        return 2;
    }

    test_line[0] = committed_c;
    memory_fence();
    cbo_clean(test_line);
    memory_fence();
    cbo_inval(test_line);
    memory_fence();
    if (test_line[0] != committed_c) {
        printf("zicbom: clean did not reach memory\n");
        return 3;
    }

    printf("zicbom: PASS\n");
    return 0;
}
