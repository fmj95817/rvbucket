#include "sdspi.h"

#include <stddef.h>

#define MMIO_WRITE32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define MMIO_READ32(addr)       (*(volatile uint32_t *)(addr))

#define SDSPI_CACHE_BLOCK_SIZE 64u
#define SDSPI_CMD0  0u
#define SDSPI_CMD8  8u
#define SDSPI_CMD17 17u
#define SDSPI_CMD18 18u
#define SDSPI_CMD24 24u
#define SDSPI_CMD25 25u
#define SDSPI_CMD55 55u
#define SDSPI_CMD58 58u
#define SDSPI_ACMD41 41u
#define SDSPI_POLL_TIMEOUT 10000000u
#define SDSPI_INIT_CLOCK_DIV 250u
#define SDSPI_DATA_CLOCK_DIV 4u

static inline void sdspi_cbo_clean(const void *addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 1"
        : : "r" (addr) : "memory");
}

static inline void sdspi_cbo_inval(const void *addr)
{
    __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 0"
        : : "r" (addr) : "memory");
}

static void sdspi_cache_clean(const void *buffer, uint32_t size)
{
    uintptr_t start = (uintptr_t)buffer & ~(SDSPI_CACHE_BLOCK_SIZE - 1u);
    uintptr_t end = ((uintptr_t)buffer + size + SDSPI_CACHE_BLOCK_SIZE - 1u) &
        ~(SDSPI_CACHE_BLOCK_SIZE - 1u);
    for (uintptr_t addr = start; addr < end; addr += SDSPI_CACHE_BLOCK_SIZE) {
        sdspi_cbo_clean((const void *)addr);
    }
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

void sdspi_dma_sync_for_cpu(void *buffer, uint32_t size)
{
    __asm__ volatile ("fence rw, rw" : : : "memory");
    uintptr_t start = (uintptr_t)buffer & ~(SDSPI_CACHE_BLOCK_SIZE - 1u);
    uintptr_t end = ((uintptr_t)buffer + size + SDSPI_CACHE_BLOCK_SIZE - 1u) &
        ~(SDSPI_CACHE_BLOCK_SIZE - 1u);
    for (uintptr_t addr = start; addr < end; addr += SDSPI_CACHE_BLOCK_SIZE) {
        sdspi_cbo_inval((const void *)addr);
    }
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

void sdspi_irq(sdspi_dev_t *dev)
{
    uint32_t status = MMIO_READ32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_STATUS);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_STATUS, status);
    dev->irq_pending |= status;
}

static int sdspi_wait(sdspi_dev_t *dev, uint32_t expected)
{
    uint32_t irq_status;
    if (dev->wait_mode == SDSPI_WAIT_POLL) {
        uint32_t timeout = SDSPI_POLL_TIMEOUT;
        do {
            irq_status = MMIO_READ32(SDSPI_BASE_ADDR +
                SDSPI_REG_IRQ_STATUS);
            if ((irq_status & (expected | SDSPI_IRQ_ERROR)) != 0) {
                break;
            }
        } while (--timeout != 0);
        if (timeout == 0) {
            return -1;
        }
        MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_STATUS, irq_status);
    } else {
        while ((dev->irq_pending & (expected | SDSPI_IRQ_ERROR)) == 0) {
            __asm__ volatile ("wfi");
        }
        irq_status = dev->irq_pending;
    }
    uint32_t cmd_status = MMIO_READ32(SDSPI_BASE_ADDR +
        SDSPI_REG_CMD_STATUS);
    uint32_t data_status = MMIO_READ32(SDSPI_BASE_ADDR +
        SDSPI_REG_DATA_STATUS);
    return ((irq_status & SDSPI_IRQ_ERROR) != 0 ||
        (cmd_status & SDSPI_CMD_STATUS_ERROR) != 0 ||
        (data_status & SDSPI_DATA_STATUS_ERROR) != 0) ? -1 : 0;
}

static int sdspi_cmd(sdspi_dev_t *dev, uint32_t opcode, uint32_t arg,
    uint32_t rsp_type)
{
    dev->irq_pending = 0;
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_STATUS, SDSPI_IRQ_MASK);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_ARG, arg);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_CTRL,
        opcode | (rsp_type << SDSPI_CMD_CTRL_RSP_SHIFT) |
        SDSPI_CMD_CTRL_START);
    return sdspi_wait(dev, SDSPI_IRQ_CMD_DONE);
}

static int sdspi_transfer(sdspi_dev_t *dev, uint32_t opcode, uint32_t lba,
    uint32_t count, const void *buffer, bool write)
{
    if (count == 0 || buffer == NULL || ((uintptr_t)buffer & 3u) != 0) {
        return -1;
    }

    dev->irq_pending = 0;
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_STATUS, SDSPI_IRQ_MASK);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_DMA_ADDR, (uintptr_t)buffer);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_BLOCK_SIZE, SDSPI_SECTOR_SIZE);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_BLOCK_COUNT, count);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_ARG, lba);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CMD_CTRL,
        opcode | (SDSPI_RSP_R1 << SDSPI_CMD_CTRL_RSP_SHIFT) |
        SDSPI_CMD_CTRL_DATA |
        (write ? SDSPI_CMD_CTRL_WRITE : 0u) | SDSPI_CMD_CTRL_START);
    return sdspi_wait(dev, SDSPI_IRQ_DATA_DONE);
}

int sdspi_init(sdspi_dev_t *dev, sdspi_wait_mode_t wait_mode)
{
    if (dev == NULL || (wait_mode != SDSPI_WAIT_IRQ &&
        wait_mode != SDSPI_WAIT_POLL)) {
        return -1;
    }
    dev->irq_pending = 0;
    dev->wait_mode = wait_mode;
    if ((MMIO_READ32(SDSPI_BASE_ADDR + SDSPI_REG_STATUS) &
        SDSPI_STATUS_CARD_PRESENT) == 0) {
        return -1;
    }
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CLOCK,
        SDSPI_INIT_CLOCK_DIV);
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CTRL, SDSPI_CTRL_ENABLE);
    uint32_t irq_enable = wait_mode == SDSPI_WAIT_IRQ ?
        SDSPI_IRQ_CMD_DONE | SDSPI_IRQ_DATA_DONE | SDSPI_IRQ_ERROR : 0;
    MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_IRQ_ENABLE, irq_enable);

    if (sdspi_cmd(dev, SDSPI_CMD0, 0, SDSPI_RSP_R1) != 0 ||
        sdspi_cmd(dev, SDSPI_CMD8, 0x1aau, SDSPI_RSP_R7) != 0 ||
        MMIO_READ32(SDSPI_BASE_ADDR + SDSPI_REG_RESP1) != 0x1aau) {
        return -1;
    }
    for (uint32_t retry = 1000u; retry != 0u; retry--) {
        if (sdspi_cmd(dev, SDSPI_CMD55, 0, SDSPI_RSP_R1) != 0 ||
            sdspi_cmd(dev, SDSPI_ACMD41, 0x40000000u,
                SDSPI_RSP_R1) != 0) {
            return -1;
        }
        if ((MMIO_READ32(SDSPI_BASE_ADDR + SDSPI_REG_RESP0) & 1u) == 0) {
            int ret = sdspi_cmd(dev, SDSPI_CMD58, 0, SDSPI_RSP_R3);
            if (ret == 0) {
                MMIO_WRITE32(SDSPI_BASE_ADDR + SDSPI_REG_CLOCK,
                    SDSPI_DATA_CLOCK_DIV);
            }
            return ret;
        }
    }
    return -1;
}

int sdspi_read_blocks_raw(sdspi_dev_t *dev, uint32_t lba, uint32_t count,
    void *buffer)
{
    return sdspi_transfer(dev, count == 1 ? SDSPI_CMD17 : SDSPI_CMD18,
        lba, count, buffer, false);
}

int sdspi_read_blocks(sdspi_dev_t *dev, uint32_t lba, uint32_t count,
    void *buffer)
{
    uint32_t size = count * SDSPI_SECTOR_SIZE;
    sdspi_cache_clean(buffer, size);
    int ret = sdspi_read_blocks_raw(dev, lba, count, buffer);
    sdspi_dma_sync_for_cpu(buffer, size);
    return ret;
}

int sdspi_write_blocks(sdspi_dev_t *dev, uint32_t lba, uint32_t count,
    const void *buffer)
{
    uint32_t size = count * SDSPI_SECTOR_SIZE;
    sdspi_cache_clean(buffer, size);
    return sdspi_transfer(dev, count == 1 ? SDSPI_CMD24 : SDSPI_CMD25,
        lba, count, buffer, true);
}
