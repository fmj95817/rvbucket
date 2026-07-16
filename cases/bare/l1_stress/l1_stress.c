#include <stdint.h>
#include <stdio.h>

#define DDR_TEST_BASE   0x41000000u
#define L1_LINE_SIZE    64u
#define L1_SET_STRIDE   (128u * L1_LINE_SIZE)
#define LINE_NUM        4u
#define CROSS_BASE_OFF  (LINE_NUM * L1_SET_STRIDE)

static volatile uint8_t *const ddr = (volatile uint8_t *)DDR_TEST_BASE;

static uint8_t pattern(unsigned line, unsigned off)
{
    return (uint8_t)(0x21u + line * 0x31u + off * 0x07u);
}

static void store_u32_unaligned(uintptr_t addr, uint32_t val)
{
    asm volatile("sw %1, 0(%0)" :: "r"(addr), "r"(val) : "memory");
}

static uint32_t load_u32_unaligned(uintptr_t addr)
{
    uint32_t val;
    asm volatile("lw %0, 0(%1)" : "=r"(val) : "r"(addr) : "memory");
    return val;
}

static volatile uint8_t *line_ptr(unsigned line)
{
    return ddr + line * L1_SET_STRIDE;
}

static volatile uint8_t *cross_ptr(void)
{
    return ddr + CROSS_BASE_OFF;
}

int main(void)
{
    printf("l1_stress: start\n");

    for (unsigned line = 0; line < LINE_NUM; line++) {
        volatile uint8_t *ptr = line_ptr(line);
        for (unsigned off = 0; off < L1_LINE_SIZE; off++)
            ptr[off] = pattern(line, off);
    }

    volatile uint8_t *cross = cross_ptr();
    for (unsigned off = 0; off < L1_LINE_SIZE * 2u; off++)
        cross[off] = (uint8_t)(0x80u + off);

    store_u32_unaligned((uintptr_t)(cross + L1_LINE_SIZE - 2u),
        0xa1b2c3d4u);

    for (unsigned line = 0; line < LINE_NUM; line++) {
        volatile uint8_t *ptr = line_ptr(line);
        for (unsigned off = 0; off < L1_LINE_SIZE; off++) {
            uint8_t exp = pattern(line, off);
            if (ptr[off] != exp) {
                printf("l1_stress: FAIL byte line=%u off=%u got=0x%02x exp=0x%02x\n",
                    line, off, ptr[off], exp);
                return 1;
            }
        }
    }

    if (cross[L1_LINE_SIZE - 2u] != 0xd4u ||
        cross[L1_LINE_SIZE - 1u] != 0xc3u ||
        cross[L1_LINE_SIZE] != 0xb2u ||
        cross[L1_LINE_SIZE + 1u] != 0xa1u) {
        printf("l1_stress: FAIL cross bytes\n");
        return 1;
    }

    uint32_t cross_word = load_u32_unaligned(
        (uintptr_t)(cross + L1_LINE_SIZE - 2u));
    if (cross_word != 0xa1b2c3d4u) {
        printf("l1_stress: FAIL cross got=0x%08x\n",
            (unsigned int)cross_word);
        return 1;
    }

    printf("l1_stress: PASS\n");
    return 0;
}
