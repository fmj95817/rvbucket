#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_NUM 48u

static int fail(const char *name)
{
    printf("heap_stress: FAIL %s\n", name);
    return 1;
}

static uint8_t pat(uint32_t block, uint32_t idx)
{
    return (uint8_t)((block * 29u + idx * 13u + 7u) & 0xffu);
}

int main(void)
{
    uint8_t *blocks[BLOCK_NUM] = {};
    size_t sizes[BLOCK_NUM] = {};

    for (uint32_t i = 0; i < BLOCK_NUM; i++) {
        sizes[i] = 128u + ((i * 97u) & 2047u);
        blocks[i] = malloc(sizes[i]);
        if (blocks[i] == NULL) {
            return fail("malloc");
        }
        for (size_t j = 0; j < sizes[i]; j++) {
            blocks[i][j] = pat(i, (uint32_t)j);
        }
    }

    for (uint32_t i = 0; i < BLOCK_NUM; i += 3u) {
        size_t old_size = sizes[i];
        sizes[i] += 4096u;
        uint8_t *new_ptr = realloc(blocks[i], sizes[i]);
        if (new_ptr == NULL) {
            return fail("realloc");
        }
        blocks[i] = new_ptr;
        for (size_t j = 0; j < old_size; j++) {
            if (blocks[i][j] != pat(i, (uint32_t)j)) {
                return fail("realloc_preserve");
            }
        }
        memset(blocks[i] + old_size, (int)i, sizes[i] - old_size);
    }

    uint32_t checksum = 0;
    for (uint32_t i = 0; i < BLOCK_NUM; i++) {
        checksum += blocks[i][0];
        checksum += blocks[i][sizes[i] - 1u];
    }
    if (checksum != 10432u) {
        return fail("checksum");
    }

    uint32_t *zero = calloc(256u, sizeof(*zero));
    if (zero == NULL) {
        return fail("calloc");
    }
    for (uint32_t i = 0; i < 256u; i++) {
        if (zero[i] != 0u) {
            return fail("calloc_zero");
        }
    }
    free(zero);

    for (uint32_t i = 0; i < BLOCK_NUM; i++) {
        free(blocks[i]);
    }

    puts("heap_stress: PASS");
    return 0;
}

