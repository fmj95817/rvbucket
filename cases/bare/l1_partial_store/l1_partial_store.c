#include <stdint.h>
#include <stdio.h>

#define DDR_BASE       0x40000000u
#define L1_LINE_SIZE   64u
#define L1_SET_STRIDE  (128u * L1_LINE_SIZE)

static volatile uint32_t *const ddr = (volatile uint32_t *)DDR_BASE;

static volatile uint32_t *line_ptr(unsigned line)
{
    return (volatile uint32_t *)((uintptr_t)ddr + line * L1_SET_STRIDE);
}

int main(void)
{
    volatile uint32_t *line0 = line_ptr(0);
    volatile uint32_t *line1 = line_ptr(1);
    volatile uint32_t *line2 = line_ptr(2);

    printf("l1_partial_store: start\n");

    for (unsigned i = 0; i < L1_LINE_SIZE / sizeof(uint32_t); i++) {
        line0[i] = 0;
        line1[i] = 0x11110000u + i;
        line2[i] = 0x22220000u + i;
    }

    line0[15] = 0x07200720u;

    for (unsigned i = 0; i < L1_LINE_SIZE / sizeof(uint32_t); i++) {
        (void)line1[i];
        (void)line2[i];
    }

    if (line0[14] != 0u) {
        printf("l1_partial_store: FAIL line0[14]=0x%08x\n",
            (unsigned int)line0[14]);
        return 1;
    }

    if (line0[15] != 0x07200720u) {
        printf("l1_partial_store: FAIL line0[15]=0x%08x\n",
            (unsigned int)line0[15]);
        return 1;
    }

    printf("l1_partial_store: PASS\n");
    return 0;
}
