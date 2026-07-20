#ifndef IO_SDSPI_H
#define IO_SDSPI_H

#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/axi4_if.h"
#include "itf/ext_irq_if.h"
#include "itf/sdspi_cmd_if.h"
#include "itf/sdspi_data_if.h"
#include "spec/io/sdspi.h"

#define SDSPI_STREAM_FIFO_WORDS 64u
#define SDSPI_WRITE_PREFETCH_WORDS 16u

typedef struct sdspi_conf {
    u32 base_addr;
    u32 size;
} sdspi_conf_t;

typedef enum sdspi_state {
    SDSPI_STATE_IDLE,
    SDSPI_STATE_WRITE_PREFETCH,
    SDSPI_STATE_SEND_CMD,
    SDSPI_STATE_WAIT_CMD_RSP,
    SDSPI_STATE_READ_STREAM,
    SDSPI_STATE_WRITE_STREAM,
    SDSPI_STATE_WAIT_WRITE_DONE
} sdspi_state_t;

typedef enum sdspi_dma_state {
    SDSPI_DMA_IDLE,
    SDSPI_DMA_TO_MEM_AW,
    SDSPI_DMA_TO_MEM_W,
    SDSPI_DMA_TO_MEM_B,
    SDSPI_DMA_TO_MEM_SYNC_AR,
    SDSPI_DMA_TO_MEM_SYNC_R,
    SDSPI_DMA_FROM_MEM_AR,
    SDSPI_DMA_FROM_MEM_R
} sdspi_dma_state_t;

typedef struct sdspi {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    AXI4_MST_DECL(dma_);
    itf_t *irq_out;
    itf_t *cmd_mst;
    itf_t *data_slv;

    u32 base_addr;
    u32 size;
    ext_irq_if_t *irq_o;

    u32 ctrl;
    u32 clock_div;
    u32 cmd_arg;
    u32 cmd_ctrl;
    u32 cmd_status;
    u32 resp0;
    u32 resp1;
    u32 dma_addr;
    u32 block_size;
    u32 block_count;
    u32 data_status;
    u32 irq_status;
    u32 irq_enable;

    bool card_present;
    bool read_only;
    bool card_idle;
    bool card_status_valid;
    bool reset_pending;
    bool config_pending;
    bool data_write;
    bool read_input_done;
    u32 data_len;
    u32 protocol_offset;

    u32 stream_fifo[SDSPI_STREAM_FIFO_WORDS];
    u32 fifo_rptr;
    u32 fifo_wptr;
    u32 fifo_count;
    u32 fifo_high_watermark;

    sdspi_state_t state;
    sdspi_dma_state_t dma_state;
    u32 dma_offset;
    u32 dma_sync_offset;
    u32 burst_bytes;
    u32 burst_beat;
    u32 burst_beats;
} sdspi_t;

extern void sdspi_construct(sdspi_t *sdspi, const char *parent,
    const char *name, const sdspi_conf_t *conf);
extern void sdspi_reset(sdspi_t *sdspi);
extern void sdspi_clock(sdspi_t *sdspi);
extern void sdspi_free(sdspi_t *sdspi);

#endif
