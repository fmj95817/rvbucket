#ifndef SPEC_IO_SDSPI_H
#define SPEC_IO_SDSPI_H

#define SDSPI_REG_VERSION       0x00u
#define SDSPI_REG_CAP           0x04u
#define SDSPI_REG_CTRL          0x08u
#define SDSPI_REG_CLOCK         0x0cu
#define SDSPI_REG_STATUS        0x10u
#define SDSPI_REG_CMD_ARG       0x14u
#define SDSPI_REG_CMD_CTRL      0x18u
#define SDSPI_REG_CMD_STATUS    0x1cu
#define SDSPI_REG_RESP0         0x20u
#define SDSPI_REG_RESP1         0x24u
#define SDSPI_REG_DMA_ADDR      0x28u
#define SDSPI_REG_BLOCK_SIZE    0x2cu
#define SDSPI_REG_BLOCK_COUNT   0x30u
#define SDSPI_REG_DATA_STATUS   0x34u
#define SDSPI_REG_IRQ_STATUS    0x38u
#define SDSPI_REG_IRQ_ENABLE    0x3cu

#define SDSPI_VERSION           0x00010000u
#define SDSPI_CAP_SPI_MODE      (1u << 0)
#define SDSPI_CAP_DMA           (1u << 1)
#define SDSPI_CAP_MAX_BLOCKS_SHIFT 16u
#define SDSPI_CAP_MAX_BLOCKS    128u

#define SDSPI_CTRL_ENABLE       (1u << 0)
#define SDSPI_CTRL_SW_RESET     (1u << 1)

#define SDSPI_STATUS_CARD_PRESENT (1u << 0)
#define SDSPI_STATUS_CARD_RO    (1u << 1)
#define SDSPI_STATUS_BUSY       (1u << 2)
#define SDSPI_STATUS_CARD_IDLE  (1u << 3)

#define SDSPI_CMD_CTRL_OPCODE_MASK 0x3fu
#define SDSPI_CMD_CTRL_RSP_SHIFT   8u
#define SDSPI_CMD_CTRL_RSP_MASK    (7u << SDSPI_CMD_CTRL_RSP_SHIFT)
#define SDSPI_CMD_CTRL_DATA        (1u << 12)
#define SDSPI_CMD_CTRL_WRITE       (1u << 13)
#define SDSPI_CMD_CTRL_START       (1u << 31)

#define SDSPI_RSP_NONE          0u
#define SDSPI_RSP_R1            1u
#define SDSPI_RSP_R1B           2u
#define SDSPI_RSP_R2            3u
#define SDSPI_RSP_R3            4u
#define SDSPI_RSP_R7            5u

#define SDSPI_CMD_STATUS_DONE       (1u << 0)
#define SDSPI_CMD_STATUS_ERROR      (1u << 1)
#define SDSPI_CMD_STATUS_TIMEOUT    (1u << 2)
#define SDSPI_CMD_STATUS_ILLEGAL    (1u << 3)
#define SDSPI_CMD_STATUS_ADDRESS    (1u << 4)
#define SDSPI_CMD_STATUS_PARAMETER  (1u << 5)

#define SDSPI_DATA_STATUS_DONE      (1u << 0)
#define SDSPI_DATA_STATUS_ERROR     (1u << 1)
#define SDSPI_DATA_STATUS_AXI_ERROR (1u << 2)
#define SDSPI_DATA_STATUS_IO_ERROR  (1u << 3)

#define SDSPI_IRQ_CMD_DONE       (1u << 0)
#define SDSPI_IRQ_DATA_DONE      (1u << 1)
#define SDSPI_IRQ_ERROR          (1u << 2)
#define SDSPI_IRQ_CARD_CHANGE    (1u << 3)
#define SDSPI_IRQ_MASK           0x0fu

#define SDSPI_SECTOR_SIZE        512u
#define SDSPI_MAX_DATA_SIZE      (SDSPI_CAP_MAX_BLOCKS * SDSPI_SECTOR_SIZE)

#endif
