#include <stdint.h>
#include <stdio.h>
#include "drivers/sdspi/sdspi.h"

#define TEST_READ_LBA   4u
#define TEST_STALE_LBA  5u
#define TEST_WRITE_LBA  7u

static volatile uint8_t dma_buffer[SDSPI_SECTOR_SIZE]
    __attribute__((aligned(64)));
static sdspi_dev_t sdspi;

uint32_t sdspi_irq_handler(void)
{
    sdspi_irq(&sdspi);
    return 4u;
}

static int check_pattern(uint8_t xor_value, uint32_t offset)
{
    for (uint32_t i = 0; i < SDSPI_SECTOR_SIZE; i++) {
        if (dma_buffer[i] != (uint8_t)((i + offset) ^ xor_value)) {
            return -1;
        }
    }
    return 0;
}

static void fill_pattern(uint8_t xor_value, uint32_t offset)
{
    for (uint32_t i = 0; i < SDSPI_SECTOR_SIZE; i++) {
        dma_buffer[i] = (uint8_t)((i + offset) ^ xor_value);
    }
}

int main(void)
{
    printf("sdspi: start\n");
    if (sdspi_init(&sdspi, SDSPI_WAIT_IRQ) != 0) {
        printf("sdspi: card initialization failed\n");
        return 1;
    }

    fill_pattern(0x5a, 0);
    if (sdspi_write_blocks(&sdspi, TEST_READ_LBA, 1,
        (const void *)dma_buffer) != 0) {
        printf("sdspi: seed write A failed\n");
        return 2;
    }
    fill_pattern(0x3c, 0);
    if (sdspi_write_blocks(&sdspi, TEST_STALE_LBA, 1,
        (const void *)dma_buffer) != 0) {
        printf("sdspi: seed write B failed\n");
        return 3;
    }

    if (sdspi_read_blocks(&sdspi, TEST_READ_LBA, 1,
        (void *)dma_buffer) != 0 ||
        check_pattern(0x5a, 0) != 0) {
        printf("sdspi: coherent read failed\n");
        return 4;
    }

    /* Confirm the test catches a non-coherent DMA read without invalidation. */
    if (sdspi_read_blocks_raw(&sdspi, TEST_STALE_LBA, 1,
        (void *)dma_buffer) != 0) {
        printf("sdspi: raw read failed\n");
        return 5;
    }
    if (check_pattern(0x3c, 0) == 0) {
        printf("sdspi: negative cache test unexpectedly passed\n");
        return 6;
    }
    sdspi_dma_sync_for_cpu((void *)dma_buffer, SDSPI_SECTOR_SIZE);
    if (check_pattern(0x3c, 0) != 0) {
        printf("sdspi: invalidate did not expose DMA data\n");
        return 7;
    }

    for (uint32_t i = 0; i < SDSPI_SECTOR_SIZE; i++) {
        dma_buffer[i] = (uint8_t)(i ^ 0xa5u);
    }
    if (sdspi_write_blocks(&sdspi, TEST_WRITE_LBA, 1,
        (const void *)dma_buffer) != 0) {
        printf("sdspi: write failed\n");
        return 8;
    }
    for (uint32_t i = 0; i < SDSPI_SECTOR_SIZE; i++) {
        dma_buffer[i] = 0;
    }
    if (sdspi_read_blocks(&sdspi, TEST_WRITE_LBA, 1,
        (void *)dma_buffer) != 0 ||
        check_pattern(0xa5, 0) != 0) {
        printf("sdspi: readback failed\n");
        return 9;
    }

    printf("sdspi: PASS\n");
    return 0;
}
