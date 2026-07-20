#ifndef SDSPI_H
#define SDSPI_H

#include <stdbool.h>
#include <stdint.h>

#define SDSPI_BASE_ADDR        0x30100000u

#define SDSPI_REG_CTRL         0x08u
#define SDSPI_REG_CLOCK        0x0cu
#define SDSPI_REG_STATUS       0x10u
#define SDSPI_REG_CMD_ARG      0x14u
#define SDSPI_REG_CMD_CTRL     0x18u
#define SDSPI_REG_CMD_STATUS   0x1cu
#define SDSPI_REG_RESP0        0x20u
#define SDSPI_REG_RESP1        0x24u
#define SDSPI_REG_DMA_ADDR     0x28u
#define SDSPI_REG_BLOCK_SIZE   0x2cu
#define SDSPI_REG_BLOCK_COUNT  0x30u
#define SDSPI_REG_DATA_STATUS  0x34u
#define SDSPI_REG_IRQ_STATUS   0x38u
#define SDSPI_REG_IRQ_ENABLE   0x3cu

#define SDSPI_CTRL_ENABLE      (1u << 0)
#define SDSPI_STATUS_CARD_PRESENT (1u << 0)

#define SDSPI_CMD_CTRL_DATA    (1u << 12)
#define SDSPI_CMD_CTRL_WRITE   (1u << 13)
#define SDSPI_CMD_CTRL_START   (1u << 31)
#define SDSPI_CMD_CTRL_RSP_SHIFT 8u

#define SDSPI_RSP_NONE         0u
#define SDSPI_RSP_R1           1u
#define SDSPI_RSP_R1B          2u
#define SDSPI_RSP_R2           3u
#define SDSPI_RSP_R3           4u
#define SDSPI_RSP_R7           5u

#define SDSPI_CMD_STATUS_ERROR (1u << 1)
#define SDSPI_DATA_STATUS_ERROR (1u << 1)

#define SDSPI_IRQ_CMD_DONE     (1u << 0)
#define SDSPI_IRQ_DATA_DONE    (1u << 1)
#define SDSPI_IRQ_ERROR        (1u << 2)
#define SDSPI_IRQ_MASK         0x0fu

#define SDSPI_SECTOR_SIZE      512u

typedef enum sdspi_wait_mode {
    SDSPI_WAIT_IRQ,
    SDSPI_WAIT_POLL
} sdspi_wait_mode_t;

typedef struct sdspi_dev {
    volatile uint32_t irq_pending;
    sdspi_wait_mode_t wait_mode;
} sdspi_dev_t;

extern int sdspi_init(sdspi_dev_t *dev, sdspi_wait_mode_t wait_mode);
extern void sdspi_irq(sdspi_dev_t *dev);
extern int sdspi_read_blocks(sdspi_dev_t *dev, uint32_t lba,
    uint32_t count, void *buffer);
extern int sdspi_write_blocks(sdspi_dev_t *dev, uint32_t lba,
    uint32_t count, const void *buffer);

/* Low-level transfer entry used by diagnostics which manage cache state. */
extern int sdspi_read_blocks_raw(sdspi_dev_t *dev, uint32_t lba,
    uint32_t count, void *buffer);
extern void sdspi_dma_sync_for_cpu(void *buffer, uint32_t size);

#endif
