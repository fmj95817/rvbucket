#include <stdint.h>
#include <stdio.h>

#define DDR_EXEC_ADDR  0x40000000u
#define DDR_DATA_ADDR  0x41000000u
#define OUTER_ITER     32u
#define DATA_LINE_NUM  1024u
#define CACHE_LINE_SIZE 64u

extern const uint32_t l1_id_payload_start[];
extern const uint32_t l1_id_payload_end[];

typedef uint32_t (*payload_fn_t)(void);

static void copy_payload(void)
{
    volatile uint32_t *dst = (volatile uint32_t *)DDR_EXEC_ADDR;
    const uint32_t *src = l1_id_payload_start;
    uint32_t words = (uint32_t)(l1_id_payload_end - l1_id_payload_start);

    for (uint32_t i = 0; i < words; i++)
        dst[i] = src[i];

    asm volatile(".word 0x0000100f" ::: "memory"); /* fence.i */
}

static uint32_t expected_xor(void)
{
    uint32_t value = 1;
    uint32_t result = 0;

    for (uint32_t outer = 0; outer < OUTER_ITER; outer++) {
        for (uint32_t line = 0; line < DATA_LINE_NUM; line++) {
            result ^= value;
            value++;
        }
    }
    return result;
}

int main(void)
{
    volatile uint32_t *data = (volatile uint32_t *)DDR_DATA_ADDR;
    uint32_t payload_size = (uint32_t)((uintptr_t)l1_id_payload_end -
        (uintptr_t)l1_id_payload_start);

    printf("l1_id_stress: payload=%u bytes\n", (unsigned int)payload_size);
    copy_payload();

    uint32_t result = ((payload_fn_t)DDR_EXEC_ADDR)();
    uint32_t final_base = 1u + (OUTER_ITER - 1u) * DATA_LINE_NUM;
    uint32_t first = data[0];
    uint32_t last = data[(DATA_LINE_NUM - 1u) *
        (CACHE_LINE_SIZE / sizeof(uint32_t))];

    if (result != expected_xor() || first != final_base ||
        last != final_base + DATA_LINE_NUM - 1u) {
        printf("l1_id_stress: FAIL xor=0x%08x first=0x%08x last=0x%08x\n",
            (unsigned int)result, (unsigned int)first, (unsigned int)last);
        return 1;
    }

    printf("l1_id_stress: PASS\n");
    return 0;
}
