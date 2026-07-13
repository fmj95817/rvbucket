#include <stdint.h>
#include <stdio.h>

#include "drivers/pcm/pcm.h"

int main(void)
{
    printf("pcm: start\n");

    uint64_t cycle0 = pcm_read(PCM_CNT_U_SOC_CYCLE);
    uint64_t exu0 =
        pcm_read(PCM_CNT_U_SOC_U_RV32G_CPU_U_HART_U_EXU_EX_REQ);
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile ("nop");
    }
    uint64_t cycle1 = pcm_read(PCM_CNT_U_SOC_CYCLE);
    uint64_t exu1 =
        pcm_read(PCM_CNT_U_SOC_U_RV32G_CPU_U_HART_U_EXU_EX_REQ);

    if (cycle1 <= cycle0) {
        printf("pcm: FAIL cycle0=%llu cycle1=%llu\n",
            (unsigned long long)cycle0, (unsigned long long)cycle1);
        return 1;
    }

    if (exu1 <= exu0) {
        printf("pcm: FAIL exu0=%llu exu1=%llu\n",
            (unsigned long long)exu0, (unsigned long long)exu1);
        return 1;
    }

    pcm_clear();

    uint64_t cycle2 = pcm_read(PCM_CNT_U_SOC_CYCLE);
    if (cycle2 > 10000u) {
        printf("pcm: FAIL clear cycle=%llu\n",
            (unsigned long long)cycle2);
        return 1;
    }

    printf("pcm: PASS cycle0=%llu cycle1=%llu cycle2=%llu exu0=%llu exu1=%llu\n",
        (unsigned long long)cycle0,
        (unsigned long long)cycle1,
        (unsigned long long)cycle2,
        (unsigned long long)exu0,
        (unsigned long long)exu1);
    return 0;
}
