#include <stdint.h>
#include <stdio.h>

static int fail_u32(const char *name, uint32_t got, uint32_t exp)
{
    printf("rv_alu_branch: FAIL %s got=0x%08x exp=0x%08x\n",
        name, got, exp);
    return 1;
}

static int fail_i32(const char *name, int32_t got, int32_t exp)
{
    printf("rv_alu_branch: FAIL %s got=%ld exp=%ld\n",
        name, (long)got, (long)exp);
    return 1;
}

static uint32_t branch_mix(void)
{
    uint32_t acc = 0x13579bdfu;

    for (uint32_t i = 0; i < 257u; i++) {
        if ((i & 7u) == 0u) {
            acc += i * 17u;
        } else if ((i & 3u) == 1u) {
            acc ^= i << (i & 15u);
        } else if (i > 128u) {
            acc -= i ^ 0x55u;
        } else {
            acc = (acc << 3) | (acc >> 29);
        }
    }

    return acc;
}

int main(void)
{
    uint32_t add = 0x12345678u + 0x11111111u;
    if (add != 0x23456789u) {
        return fail_u32("add", add, 0x23456789u);
    }

    uint32_t mul = 0x12345u * 0x321u;
    if (mul != 0x038f5ae5u) {
        return fail_u32("mul", mul, 0x038f5ae5u);
    }

    int32_t div = -123456789 / 12345;
    int32_t rem = -123456789 % 12345;
    if (div != -10000) {
        return fail_i32("div", div, -10000);
    }
    if (rem != -6789) {
        return fail_i32("rem", rem, -6789);
    }

    uint32_t shifts = ((0x80000000u >> 31) << 8) | (0x00000081u >> 1);
    if (shifts != 0x00000140u) {
        return fail_u32("shifts", shifts, 0x00000140u);
    }

    if (!((int32_t)0x80000000u < 0) || !((uint32_t)-1 > 1u)) {
        return fail_u32("compare", 0u, 1u);
    }

    uint32_t mix = branch_mix();
    if (mix != 0x5954b178u) {
        return fail_u32("branch_mix", mix, 0x5954b178u);
    }

    puts("rv_alu_branch: PASS");
    return 0;
}

