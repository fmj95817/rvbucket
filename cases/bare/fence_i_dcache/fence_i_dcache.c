#include <stdint.h>
#include <stdio.h>

#define CODE_ADDR     0x40400000u
#define L1_LINE_SIZE  64u
#define L1_SET_STRIDE (128u * L1_LINE_SIZE)

typedef int (*test_func_t)(void);

static volatile uint32_t *const code = (volatile uint32_t *)CODE_ADDR;

static void install_code(uint32_t ret_val)
{
    code[0] = 0x00000513u | ((ret_val & 0xfffu) << 20); /* addi a0, zero, imm */
    code[1] = 0x00008067u;                              /* ret */
}

static void evict_code_line(void)
{
    volatile uint32_t *base = (volatile uint32_t *)CODE_ADDR;

    for (unsigned line = 1; line <= 3; line++) {
        volatile uint32_t *ptr =
            (volatile uint32_t *)((uintptr_t)base + line * L1_SET_STRIDE);
        for (unsigned word = 0; word < L1_LINE_SIZE / sizeof(uint32_t); word++)
            ptr[word] = 0xdead0000u | (line << 8) | word;
    }
}

static int call_code(void)
{
    test_func_t func = (test_func_t)CODE_ADDR;
    return func();
}

int main(void)
{
    printf("fence_i_dcache: start\n");

    install_code(1);
    evict_code_line();
    asm volatile(".word 0x0000100f" ::: "memory"); /* fence.i */
    if (call_code() != 1) {
        printf("fence_i_dcache: FAIL initial\n");
        return 1;
    }

    install_code(2);
    asm volatile(".word 0x0000100f" ::: "memory"); /* fence.i */
    if (call_code() != 2) {
        printf("fence_i_dcache: FAIL stale\n");
        return 1;
    }

    printf("fence_i_dcache: PASS\n");
    return 0;
}
