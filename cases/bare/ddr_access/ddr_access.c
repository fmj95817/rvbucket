#include <stdint.h>
#include <stdio.h>

#define DDR_BASE 0x40000000u

int main(void)
{
    volatile uint32_t *ddr = (volatile uint32_t *)DDR_BASE;

    printf("ddr_access: start\n");
    ddr[0] = 0x11223344u;
    printf("ddr_access: w0\n");
    ddr[1] = 0xa5a55a5au;
    printf("ddr_access: w1\n");
    ddr[1024] = ddr[0] ^ ddr[1];
    printf("ddr_access: w2\n");

    if (ddr[0] != 0x11223344u ||
        ddr[1] != 0xa5a55a5au ||
        ddr[1024] != 0xb487691eu) {
        printf("ddr_access: FAIL v0=0x%08x v1=0x%08x v2=0x%08x\n",
            (unsigned int)ddr[0], (unsigned int)ddr[1],
            (unsigned int)ddr[1024]);
        return 1;
    }

    printf("ddr_access: PASS\n");
    return 0;
}
