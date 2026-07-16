#include <stdio.h>

static volatile unsigned amo_value __attribute__((aligned(64))) = 5;
static volatile unsigned swap_value __attribute__((aligned(64)));

int main(void)
{
    unsigned add = 3;
    unsigned old;
    unsigned sc;

    printf("amo: start\n");
    printf("amo: ddr amoadd\n");

    __asm__ volatile ("amoadd.w %0, %2, (%1)"
        : "=r"(old) : "r"(&amo_value), "r"(add) : "memory");
    if (old != 5 || amo_value != 8) {
        printf("amoadd: FAIL\n");
        return 1;
    }

    printf("amo: ddr lrsc\n");

    __asm__ volatile (
        "lr.w %0, (%2)\n"
        "addi %0, %0, 4\n"
        "sc.w %1, %0, (%2)"
        : "=&r"(old), "=&r"(sc) : "r"(&amo_value) : "memory");
    if (sc != 0 || amo_value != 12) {
        printf("lrsc: FAIL\n");
        return 1;
    }

    printf("amo: ddr amoswap\n");

    swap_value = 0;
    __asm__ volatile ("amoswap.w %0, %2, (%1)"
        : "=r"(old) : "r"(&swap_value), "r"(1u) : "memory");
    if (old != 0 || swap_value != 1) {
        printf("ddr amoswap: FAIL\n");
        return 1;
    }

    printf("amo: PASS\n");
    return 0;
}
