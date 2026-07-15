#include "pcm.h"

static volatile uint32_t *pcm_reg(uint32_t offset)
{
    return (volatile uint32_t *)(PCM_BASE_ADDR + offset);
}

void pcm_clear(void)
{
    *pcm_reg(PCM_REG_CLEAR) = 1u;
}

uint64_t pcm_read(uint32_t idx)
{
    volatile uint32_t *lo = pcm_reg(idx * 8u);
    volatile uint32_t *hi = pcm_reg(idx * 8u + 4u);
    uint32_t hi0;
    uint32_t low;
    uint32_t hi1;

    do {
        hi0 = *hi;
        low = *lo;
        hi1 = *hi;
    } while (hi0 != hi1);

    return ((uint64_t)hi1 << 32) | low;
}
