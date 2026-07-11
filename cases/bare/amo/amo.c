#include <stdio.h>

int main(void)
{
    volatile unsigned value = 5;
    unsigned add = 3;
    unsigned old;
    unsigned sc;

    printf("amo: start\n");
    printf("amo: dtcm amoadd\n");

    __asm__ volatile ("amoadd.w %0, %2, (%1)"
        : "=r"(old) : "r"(&value), "r"(add) : "memory");
    if (old != 5 || value != 8) {
        printf("amoadd: FAIL\n");
        return 1;
    }

    printf("amo: dtcm lrsc\n");

    __asm__ volatile (
        "lr.w %0, (%2)\n"
        "addi %0, %0, 4\n"
        "sc.w %1, %0, (%2)"
        : "=&r"(old), "=&r"(sc) : "r"(&value) : "memory");
    if (sc != 0 || value != 12) {
        printf("lrsc: FAIL\n");
        return 1;
    }

    printf("amo: itcm amoswap\n");

    volatile unsigned *itcm_amo = (volatile unsigned *)0x10040068u;
    *itcm_amo = 0;
    __asm__ volatile ("amoswap.w %0, %2, (%1)"
        : "=r"(old) : "r"(itcm_amo), "r"(1u) : "memory");
    if (old != 0 || *itcm_amo != 1) {
        printf("itcm amoswap: FAIL\n");
        return 1;
    }

    printf("amo: PASS\n");
    return 0;
}
