#include <stdint.h>
#include <stdio.h>

static int fail(const char *name, uint32_t got, uint32_t exp)
{
    printf("unaligned_access: FAIL %s got=0x%08x exp=0x%08x\n",
        name, got, exp);
    return 1;
}

static uint32_t unaligned_lw(const void *ptr)
{
    uint32_t val;
    __asm__ volatile ("lw %0, 0(%1)"
        : "=r"(val)
        : "r"(ptr)
        : "memory");
    return val;
}

static void unaligned_sw(void *ptr, uint32_t val)
{
    __asm__ volatile ("sw %1, 0(%0)"
        :
        : "r"(ptr), "r"(val)
        : "memory");
}

int main(void)
{
    uint8_t buf[16] __attribute__((aligned(4))) = {};

    buf[1] = 0x44u;
    buf[2] = 0x33u;
    buf[3] = 0x22u;
    buf[4] = 0x11u;
    uint32_t val = unaligned_lw(&buf[1]);
    if (val != 0x11223344u) {
        return fail("lw", val, 0x11223344u);
    }

    unaligned_sw(&buf[5], 0xa1b2c3d4u);
    uint32_t assembled = (uint32_t)buf[5] |
        ((uint32_t)buf[6] << 8) |
        ((uint32_t)buf[7] << 16) |
        ((uint32_t)buf[8] << 24);
    if (assembled != 0xa1b2c3d4u) {
        return fail("sw", assembled, 0xa1b2c3d4u);
    }

    puts("unaligned_access: PASS");
    return 0;
}

