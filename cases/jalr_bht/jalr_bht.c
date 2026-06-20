#include <stdint.h>
#include <stdio.h>

typedef int (*target_fn_t)(void);

static volatile int g_last_target;
static volatile int g_accum;
static volatile int g_arg;

__attribute__((noinline)) static int target_a(void)
{
    if (g_last_target != 2 && g_last_target != 0) {
        g_accum = -1000;
    }
    g_last_target = 1;
    g_accum += g_arg + 3;
    return g_accum;
}

__attribute__((noinline)) static int target_b(void)
{
    if (g_last_target != 1) {
        g_accum = -2000;
    }
    g_last_target = 2;
    g_accum += g_arg + 7;
    return g_accum;
}

__attribute__((noinline)) static int jalr_call(target_fn_t fn)
{
    register uintptr_t a0 asm("a0") = (uintptr_t)fn;

    asm volatile (
        "jalr ra, 0(a0)\n"
        : "+r"(a0)
        :
        : "ra", "memory"
    );

    return (int)a0;
}

int main(void)
{
    int expected = 0;

    for (int i = 0; i < 128; i++) {
        target_fn_t fn;
        int delta;

        if ((i & 1) == 0) {
            fn = target_a;
            delta = i + 3;
        } else {
            fn = target_b;
            delta = i + 7;
        }

        expected += delta;
        g_arg = i;
        int got = jalr_call(fn);
        if (got != expected || g_accum != expected) {
            printf("jalr_bht: FAIL i=%d got=%d accum=%d expected=%d last=%d\n",
                i, got, g_accum, expected, g_last_target);
            return 1;
        }
    }

    printf("jalr_bht: PASS accum=%d\n", g_accum);
    return 0;
}
