#include "sdspi.h"

#include <string.h>
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define SDSPI_DMA_ID 0xffu
#define SDSPI_DMA_MAX_BURST_BEATS 16u

static u32 sdspi_merge_write(u32 old, u32 value, u8 strobe)
{
    for (u32 i = 0; i < 4; i++) {
        if ((strobe & (1u << i)) != 0) {
            u32 mask = 0xffu << (i * 8u);
            old = (old & ~mask) | (value & mask);
        }
    }
    return old;
}

static void sdspi_fifo_reset(sdspi_t *sdspi)
{
    sdspi->fifo_rptr = 0;
    sdspi->fifo_wptr = 0;
    sdspi->fifo_count = 0;
    sdspi->fifo_high_watermark = 0;
}

static u32 sdspi_fifo_free(const sdspi_t *sdspi)
{
    return SDSPI_STREAM_FIFO_WORDS - sdspi->fifo_count;
}

static void sdspi_fifo_push(sdspi_t *sdspi, u32 data)
{
    DBG_CHECK(sdspi->fifo_count < SDSPI_STREAM_FIFO_WORDS);
    sdspi->stream_fifo[sdspi->fifo_wptr] = data;
    sdspi->fifo_wptr = (sdspi->fifo_wptr + 1u) % SDSPI_STREAM_FIFO_WORDS;
    sdspi->fifo_count++;
    if (sdspi->fifo_count > sdspi->fifo_high_watermark) {
        sdspi->fifo_high_watermark = sdspi->fifo_count;
    }
}

static u32 sdspi_fifo_front(const sdspi_t *sdspi)
{
    DBG_CHECK(sdspi->fifo_count != 0);
    return sdspi->stream_fifo[sdspi->fifo_rptr];
}

static void sdspi_fifo_pop(sdspi_t *sdspi)
{
    DBG_CHECK(sdspi->fifo_count != 0);
    sdspi->fifo_rptr = (sdspi->fifo_rptr + 1u) % SDSPI_STREAM_FIFO_WORDS;
    sdspi->fifo_count--;
}

static void sdspi_update_irq(sdspi_t *sdspi)
{
    bool irq = (sdspi->irq_status & sdspi->irq_enable) != 0;
    if (sdspi->irq_o->irq != irq) {
        sdspi->irq_o->irq = irq;
        itf_signal_write_notify(sdspi->irq_out);
    }
}

static u32 sdspi_status(const sdspi_t *sdspi)
{
    u32 status = 0;
    if (sdspi->card_present) {
        status |= SDSPI_STATUS_CARD_PRESENT;
    }
    if (sdspi->read_only) {
        status |= SDSPI_STATUS_CARD_RO;
    }
    if (sdspi->state != SDSPI_STATE_IDLE ||
        sdspi->dma_state != SDSPI_DMA_IDLE || sdspi->reset_pending) {
        status |= SDSPI_STATUS_BUSY;
    }
    if (sdspi->card_idle) {
        status |= SDSPI_STATUS_CARD_IDLE;
    }
    return status;
}

static void sdspi_reset_regs(sdspi_t *sdspi)
{
    sdspi->ctrl = 0;
    sdspi->clock_div = 0;
    sdspi->cmd_arg = 0;
    sdspi->cmd_ctrl = 0;
    sdspi->cmd_status = 0;
    sdspi->resp0 = 0;
    sdspi->resp1 = 0;
    sdspi->dma_addr = 0;
    sdspi->block_size = SDSPI_SECTOR_SIZE;
    sdspi->block_count = 1;
    sdspi->data_status = 0;
    sdspi->irq_status = 0;
    sdspi->irq_enable = 0;
    sdspi->config_pending = true;
    sdspi->data_write = false;
    sdspi->read_input_done = false;
    sdspi->data_len = 0;
    sdspi->protocol_offset = 0;
    sdspi->state = SDSPI_STATE_IDLE;
    sdspi->dma_state = SDSPI_DMA_IDLE;
    sdspi->dma_offset = 0;
    sdspi->dma_sync_offset = 0;
    sdspi->burst_bytes = 0;
    sdspi->burst_beat = 0;
    sdspi->burst_beats = 0;
    sdspi_fifo_reset(sdspi);
    sdspi->irq_o->irq = false;
    itf_signal_write_notify(sdspi->irq_out);
}

void sdspi_construct(sdspi_t *sdspi, const char *parent, const char *name,
    const sdspi_conf_t *conf)
{
    mod_construct(&sdspi->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf != NULL);
    DBG_CHECK(conf->size >= 0x40u);
    DBG_CHECK(sdspi->cmd_mst != NULL);
    DBG_CHECK(sdspi->data_slv != NULL);

    sdspi->base_addr = conf->base_addr;
    sdspi->size = conf->size;
    sdspi->card_present = false;
    sdspi->read_only = false;
    sdspi->card_idle = true;
    sdspi->card_status_valid = false;
    sdspi->reset_pending = false;
    sdspi->irq_o = itf_signal_get_src_and_chk(sdspi->irq_out);

    dbg_vcd_add_sig("cmd_status", DBG_SIG_TYPE_REG, 32,
        &sdspi->cmd_status);
    dbg_vcd_add_sig("data_status", DBG_SIG_TYPE_REG, 32,
        &sdspi->data_status);
    dbg_vcd_add_sig("irq_status", DBG_SIG_TYPE_REG, 32,
        &sdspi->irq_status);
    dbg_vcd_add_sig("state", DBG_SIG_TYPE_REG, 32, &sdspi->state);
    dbg_vcd_add_sig("dma_state", DBG_SIG_TYPE_REG, 32,
        &sdspi->dma_state);
    dbg_vcd_add_sig("fifo_count", DBG_SIG_TYPE_REG, 32,
        &sdspi->fifo_count);
}

void sdspi_reset(sdspi_t *sdspi)
{
    mod_reset(&sdspi->mod);
    sdspi->card_present = false;
    sdspi->read_only = false;
    sdspi->card_idle = true;
    sdspi->card_status_valid = false;
    sdspi->reset_pending = false;
    sdspi_reset_regs(sdspi);
}

static void sdspi_finish(sdspi_t *sdspi, bool data, bool error,
    u32 cmd_error, u32 data_error)
{
    sdspi->state = SDSPI_STATE_IDLE;
    sdspi->dma_state = SDSPI_DMA_IDLE;
    sdspi_fifo_reset(sdspi);
    sdspi->cmd_status = SDSPI_CMD_STATUS_DONE | cmd_error;
    sdspi->data_status = data ? SDSPI_DATA_STATUS_DONE | data_error : 0;
    sdspi->irq_status |= SDSPI_IRQ_CMD_DONE;
    if (data) {
        sdspi->irq_status |= SDSPI_IRQ_DATA_DONE;
    }
    if (error) {
        sdspi->cmd_status |= SDSPI_CMD_STATUS_ERROR;
        if (data) {
            sdspi->data_status |= SDSPI_DATA_STATUS_ERROR;
        }
        sdspi->irq_status |= SDSPI_IRQ_ERROR;
    }
    sdspi_update_irq(sdspi);
}

static bool sdspi_start_cmd(sdspi_t *sdspi, u32 cmd_ctrl)
{
    bool has_data = (cmd_ctrl & SDSPI_CMD_CTRL_DATA) != 0;
    bool write = (cmd_ctrl & SDSPI_CMD_CTRL_WRITE) != 0;
    if (sdspi->state != SDSPI_STATE_IDLE ||
        sdspi->dma_state != SDSPI_DMA_IDLE || sdspi->reset_pending) {
        return false;
    }

    sdspi->cmd_ctrl = cmd_ctrl & ~SDSPI_CMD_CTRL_START;
    sdspi->cmd_status = 0;
    sdspi->data_status = 0;
    sdspi->resp0 = 0;
    sdspi->resp1 = 0;

    if ((sdspi->ctrl & SDSPI_CTRL_ENABLE) == 0 ||
        !sdspi->card_present) {
        sdspi_finish(sdspi, has_data, true, SDSPI_CMD_STATUS_TIMEOUT, 0);
        return true;
    }
    if (write && !has_data) {
        sdspi_finish(sdspi, false, true, SDSPI_CMD_STATUS_PARAMETER, 0);
        return true;
    }

    sdspi->data_write = write;
    if (has_data) {
        u64 length = (u64)sdspi->block_size * sdspi->block_count;
        if (length == 0 || length > SDSPI_MAX_DATA_SIZE ||
            (sdspi->dma_addr & 3u) != 0 || (length & 3u) != 0) {
            sdspi_finish(sdspi, true, true, SDSPI_CMD_STATUS_PARAMETER, 0);
            return true;
        }
        sdspi->data_len = (u32)length;
    } else {
        sdspi->data_len = 0;
    }

    sdspi_fifo_reset(sdspi);
    sdspi->read_input_done = false;
    sdspi->protocol_offset = 0;
    sdspi->dma_offset = 0;
    sdspi->dma_sync_offset = 0;
    sdspi->dma_state = SDSPI_DMA_IDLE;
    sdspi->state = write ? SDSPI_STATE_WRITE_PREFETCH :
        SDSPI_STATE_SEND_CMD;
    return true;
}

static bool sdspi_read_reg(sdspi_t *sdspi, u32 offset, u32 *value)
{
    switch (offset) {
    case SDSPI_REG_VERSION:
        *value = SDSPI_VERSION;
        return true;
    case SDSPI_REG_CAP:
        *value = SDSPI_CAP_SPI_MODE | SDSPI_CAP_DMA |
            (SDSPI_CAP_MAX_BLOCKS << SDSPI_CAP_MAX_BLOCKS_SHIFT);
        return true;
    case SDSPI_REG_CTRL:
        *value = sdspi->ctrl;
        return true;
    case SDSPI_REG_CLOCK:
        *value = sdspi->clock_div;
        return true;
    case SDSPI_REG_STATUS:
        *value = sdspi_status(sdspi);
        return true;
    case SDSPI_REG_CMD_ARG:
        *value = sdspi->cmd_arg;
        return true;
    case SDSPI_REG_CMD_CTRL:
        *value = sdspi->cmd_ctrl;
        return true;
    case SDSPI_REG_CMD_STATUS:
        *value = sdspi->cmd_status;
        return true;
    case SDSPI_REG_RESP0:
        *value = sdspi->resp0;
        return true;
    case SDSPI_REG_RESP1:
        *value = sdspi->resp1;
        return true;
    case SDSPI_REG_DMA_ADDR:
        *value = sdspi->dma_addr;
        return true;
    case SDSPI_REG_BLOCK_SIZE:
        *value = sdspi->block_size;
        return true;
    case SDSPI_REG_BLOCK_COUNT:
        *value = sdspi->block_count;
        return true;
    case SDSPI_REG_DATA_STATUS:
        *value = sdspi->data_status;
        return true;
    case SDSPI_REG_IRQ_STATUS:
        *value = sdspi->irq_status;
        return true;
    case SDSPI_REG_IRQ_ENABLE:
        *value = sdspi->irq_enable;
        return true;
    default:
        return false;
    }
}

static bool sdspi_busy(const sdspi_t *sdspi)
{
    return sdspi->state != SDSPI_STATE_IDLE ||
        sdspi->dma_state != SDSPI_DMA_IDLE || sdspi->reset_pending;
}

static bool sdspi_write_reg(sdspi_t *sdspi, u32 offset, u32 value, u8 strobe)
{
    switch (offset) {
    case SDSPI_REG_CTRL: {
        u32 ctrl = sdspi_merge_write(sdspi->ctrl, value, strobe);
        if ((ctrl & SDSPI_CTRL_SW_RESET) != 0) {
            sdspi_reset_regs(sdspi);
            sdspi->reset_pending = true;
            return true;
        }
        if (sdspi_busy(sdspi)) {
            return false;
        }
        ctrl &= SDSPI_CTRL_ENABLE;
        if (ctrl != sdspi->ctrl) {
            sdspi->ctrl = ctrl;
            sdspi->config_pending = true;
        }
        return true;
    }
    case SDSPI_REG_CLOCK: {
        if (sdspi_busy(sdspi)) {
            return false;
        }
        u32 clock_div = sdspi_merge_write(sdspi->clock_div, value, strobe);
        if (clock_div != sdspi->clock_div) {
            sdspi->clock_div = clock_div;
            sdspi->config_pending = true;
        }
        return true;
    }
    case SDSPI_REG_CMD_ARG:
        sdspi->cmd_arg = sdspi_merge_write(sdspi->cmd_arg, value, strobe);
        return true;
    case SDSPI_REG_CMD_CTRL: {
        u32 ctrl = sdspi_merge_write(sdspi->cmd_ctrl, value, strobe);
        if ((ctrl & SDSPI_CMD_CTRL_START) != 0) {
            return sdspi_start_cmd(sdspi, ctrl);
        }
        sdspi->cmd_ctrl = ctrl;
        return true;
    }
    case SDSPI_REG_DMA_ADDR:
        sdspi->dma_addr = sdspi_merge_write(sdspi->dma_addr, value, strobe);
        return true;
    case SDSPI_REG_BLOCK_SIZE:
        sdspi->block_size = sdspi_merge_write(sdspi->block_size, value,
            strobe);
        return true;
    case SDSPI_REG_BLOCK_COUNT:
        sdspi->block_count = sdspi_merge_write(sdspi->block_count, value,
            strobe);
        return true;
    case SDSPI_REG_IRQ_STATUS:
        sdspi->irq_status &= ~(value & SDSPI_IRQ_MASK);
        sdspi_update_irq(sdspi);
        return true;
    case SDSPI_REG_IRQ_ENABLE:
        sdspi->irq_enable = sdspi_merge_write(sdspi->irq_enable, value,
            strobe) & SDSPI_IRQ_MASK;
        sdspi_update_irq(sdspi);
        return true;
    default:
        return false;
    }
}

static void sdspi_apb_proc(sdspi_t *sdspi)
{
    if (itf_fifo_empty(sdspi->apb_req_slv) ||
        itf_fifo_full(sdspi->apb_rsp_mst)) {
        return;
    }

    apb_req_if_t req;
    itf_read(sdspi->apb_req_slv, &req);
    apb_rsp_if_t rsp = { .pslverr = false, .prdata = 0 };
    if (req.paddr < sdspi->base_addr ||
        req.paddr >= sdspi->base_addr + sdspi->size ||
        (req.paddr & 3u) != 0) {
        rsp.pslverr = true;
    } else {
        u32 offset = req.paddr - sdspi->base_addr;
        bool ok = req.pwrite ? sdspi_write_reg(sdspi, offset, req.pwdata,
            req.pstrb) : sdspi_read_reg(sdspi, offset, &rsp.prdata);
        rsp.pslverr = !ok;
    }
    itf_write(sdspi->apb_rsp_mst, &rsp);
}

static void sdspi_dma_error(sdspi_t *sdspi)
{
    sdspi->reset_pending = true;
    sdspi_finish(sdspi, true, true, 0, SDSPI_DATA_STATUS_AXI_ERROR);
}

static void sdspi_prepare_burst(sdspi_t *sdspi, u32 offset, u32 max_beats)
{
    DBG_CHECK(max_beats != 0);
    u32 addr = sdspi->dma_addr + offset;
    u32 remaining = sdspi->data_len - offset;
    u32 boundary = 0x1000u - (addr & 0xfffu);
    u32 max_bytes = max_beats * sizeof(u32);
    sdspi->burst_bytes = remaining;
    if (sdspi->burst_bytes > boundary) {
        sdspi->burst_bytes = boundary;
    }
    if (sdspi->burst_bytes > max_bytes) {
        sdspi->burst_bytes = max_bytes;
    }
    sdspi->burst_beats = sdspi->burst_bytes / sizeof(u32);
    sdspi->burst_beat = 0;
    DBG_CHECK(sdspi->burst_beats != 0);
}

static void sdspi_begin_burst(sdspi_t *sdspi, bool to_mem, u32 max_beats)
{
    sdspi_prepare_burst(sdspi, sdspi->dma_offset, max_beats);
    sdspi->dma_state = to_mem ? SDSPI_DMA_TO_MEM_AW :
        SDSPI_DMA_FROM_MEM_AR;
}

static void sdspi_dma_to_mem(sdspi_t *sdspi)
{
    if (sdspi->dma_state == SDSPI_DMA_TO_MEM_AW) {
        if (itf_fifo_full(sdspi->dma_axi4_aw_mst)) {
            return;
        }
        axi4_aw_if_t aw = {
            .id = SDSPI_DMA_ID,
            .addr = sdspi->dma_addr + sdspi->dma_offset,
            .len = (u8)(sdspi->burst_beats - 1u),
            .size = AXI4_AW_SIZE_B4,
            .burst = AXI4_AW_BURST_INCR,
            .lock = false,
            .cache = 0,
            .prot = 0,
            .qos = 0,
            .user = 0
        };
        itf_write(sdspi->dma_axi4_aw_mst, &aw);
        sdspi->dma_state = SDSPI_DMA_TO_MEM_W;
        return;
    }

    if (sdspi->dma_state == SDSPI_DMA_TO_MEM_W) {
        if (sdspi->fifo_count == 0 ||
            itf_fifo_full(sdspi->dma_axi4_w_mst)) {
            return;
        }
        axi4_w_if_t w = {
            .data = sdspi_fifo_front(sdspi),
            .strb = 0x0fu,
            .last = sdspi->burst_beat + 1u == sdspi->burst_beats
        };
        itf_write(sdspi->dma_axi4_w_mst, &w);
        sdspi_fifo_pop(sdspi);
        sdspi->burst_beat++;
        if (w.last) {
            sdspi->dma_state = SDSPI_DMA_TO_MEM_B;
        }
        return;
    }

    if (sdspi->dma_state == SDSPI_DMA_TO_MEM_B &&
        !itf_fifo_empty(sdspi->dma_axi4_b_slv)) {
        axi4_b_if_t b;
        itf_read(sdspi->dma_axi4_b_slv, &b);
        if (b.id != SDSPI_DMA_ID || b.resp != AXI4_B_RESP_OKAY) {
            sdspi_dma_error(sdspi);
            return;
        }
        sdspi->dma_offset += sdspi->burst_bytes;
        if (sdspi->dma_offset == sdspi->data_len) {
            sdspi->dma_sync_offset = 0;
            sdspi_prepare_burst(sdspi, 0, SDSPI_DMA_MAX_BURST_BEATS);
            sdspi->dma_state = SDSPI_DMA_TO_MEM_SYNC_AR;
        } else {
            sdspi->dma_state = SDSPI_DMA_IDLE;
        }
        return;
    }

    if (sdspi->dma_state == SDSPI_DMA_TO_MEM_SYNC_AR) {
        if (itf_fifo_full(sdspi->dma_axi4_ar_mst)) {
            return;
        }
        axi4_ar_if_t ar = {
            .id = SDSPI_DMA_ID,
            .addr = sdspi->dma_addr + sdspi->dma_sync_offset,
            .len = (u8)(sdspi->burst_beats - 1u),
            .size = AXI4_AR_SIZE_B4,
            .burst = AXI4_AR_BURST_INCR,
            .lock = false,
            .cache = 0,
            .prot = 0,
            .qos = 0,
            .user = 0
        };
        itf_write(sdspi->dma_axi4_ar_mst, &ar);
        sdspi->dma_state = SDSPI_DMA_TO_MEM_SYNC_R;
        return;
    }

    if (sdspi->dma_state == SDSPI_DMA_TO_MEM_SYNC_R &&
        !itf_fifo_empty(sdspi->dma_axi4_r_slv)) {
        axi4_r_if_t r;
        itf_read(sdspi->dma_axi4_r_slv, &r);
        bool expected_last = sdspi->burst_beat + 1u == sdspi->burst_beats;
        if (r.id != SDSPI_DMA_ID || r.resp != AXI4_R_RESP_OKAY ||
            r.last != expected_last) {
            sdspi_dma_error(sdspi);
            return;
        }
        sdspi->burst_beat++;
        if (r.last) {
            sdspi->dma_sync_offset += sdspi->burst_bytes;
            if (sdspi->dma_sync_offset == sdspi->data_len) {
                if (!sdspi->read_input_done) {
                    sdspi->reset_pending = true;
                    sdspi_finish(sdspi, true, true, 0,
                        SDSPI_DATA_STATUS_IO_ERROR);
                } else {
                    sdspi_finish(sdspi, true, false, 0, 0);
                }
            } else {
                sdspi_prepare_burst(sdspi, sdspi->dma_sync_offset,
                    SDSPI_DMA_MAX_BURST_BEATS);
                sdspi->dma_state = SDSPI_DMA_TO_MEM_SYNC_AR;
            }
        }
    }
}

static void sdspi_dma_from_mem(sdspi_t *sdspi)
{
    if (sdspi->dma_state == SDSPI_DMA_FROM_MEM_AR) {
        if (itf_fifo_full(sdspi->dma_axi4_ar_mst)) {
            return;
        }
        axi4_ar_if_t ar = {
            .id = SDSPI_DMA_ID,
            .addr = sdspi->dma_addr + sdspi->dma_offset,
            .len = (u8)(sdspi->burst_beats - 1u),
            .size = AXI4_AR_SIZE_B4,
            .burst = AXI4_AR_BURST_INCR,
            .lock = false,
            .cache = 0,
            .prot = 0,
            .qos = 0,
            .user = 0
        };
        itf_write(sdspi->dma_axi4_ar_mst, &ar);
        sdspi->dma_state = SDSPI_DMA_FROM_MEM_R;
        return;
    }

    if (sdspi->dma_state == SDSPI_DMA_FROM_MEM_R &&
        !itf_fifo_empty(sdspi->dma_axi4_r_slv)) {
        if (sdspi->fifo_count == SDSPI_STREAM_FIFO_WORDS) {
            return;
        }
        axi4_r_if_t r;
        itf_read(sdspi->dma_axi4_r_slv, &r);
        bool expected_last = sdspi->burst_beat + 1u == sdspi->burst_beats;
        if (r.id != SDSPI_DMA_ID || r.resp != AXI4_R_RESP_OKAY ||
            r.last != expected_last) {
            sdspi_dma_error(sdspi);
            return;
        }
        sdspi_fifo_push(sdspi, r.data);
        sdspi->burst_beat++;
        if (r.last) {
            sdspi->dma_offset += sdspi->burst_bytes;
            sdspi->dma_state = SDSPI_DMA_IDLE;
        }
    }
}

static void sdspi_dma_proc(sdspi_t *sdspi)
{
    switch (sdspi->dma_state) {
    case SDSPI_DMA_TO_MEM_AW:
    case SDSPI_DMA_TO_MEM_W:
    case SDSPI_DMA_TO_MEM_B:
    case SDSPI_DMA_TO_MEM_SYNC_AR:
    case SDSPI_DMA_TO_MEM_SYNC_R:
        sdspi_dma_to_mem(sdspi);
        break;
    case SDSPI_DMA_FROM_MEM_AR:
    case SDSPI_DMA_FROM_MEM_R:
        sdspi_dma_from_mem(sdspi);
        break;
    case SDSPI_DMA_IDLE:
        break;
    }
}

static void sdspi_update_prefetch(sdspi_t *sdspi)
{
    if (sdspi->state != SDSPI_STATE_WRITE_PREFETCH) {
        return;
    }
    u32 target = sdspi->data_len / sizeof(u32);
    if (target > SDSPI_WRITE_PREFETCH_WORDS) {
        target = SDSPI_WRITE_PREFETCH_WORDS;
    }
    if (sdspi->fifo_count >= target ||
        (sdspi->dma_offset == sdspi->data_len &&
         sdspi->dma_state == SDSPI_DMA_IDLE)) {
        sdspi->state = SDSPI_STATE_SEND_CMD;
    }
}

static void sdspi_dma_schedule(sdspi_t *sdspi)
{
    if (sdspi->dma_state != SDSPI_DMA_IDLE ||
        sdspi->dma_offset == sdspi->data_len) {
        return;
    }

    if (sdspi->state == SDSPI_STATE_WRITE_PREFETCH ||
        sdspi->state == SDSPI_STATE_WRITE_STREAM) {
        u32 free_words = sdspi_fifo_free(sdspi);
        if (free_words != 0) {
            u32 beats = free_words;
            if (beats > SDSPI_DMA_MAX_BURST_BEATS) {
                beats = SDSPI_DMA_MAX_BURST_BEATS;
            }
            sdspi_begin_burst(sdspi, false, beats);
        }
    } else if (sdspi->state == SDSPI_STATE_READ_STREAM &&
        sdspi->fifo_count != 0) {
        sdspi_begin_burst(sdspi, true, SDSPI_DMA_MAX_BURST_BEATS);
    }
}

static bool sdspi_send_reset(sdspi_t *sdspi)
{
    if (itf_fifo_full(sdspi->cmd_mst)) {
        return false;
    }
    sdspi_cmd_if_t pkt = { .kind = SDSPI_CMD_KIND_RESET };
    itf_write(sdspi->cmd_mst, &pkt);
    sdspi->reset_pending = false;
    return true;
}

static bool sdspi_send_config(sdspi_t *sdspi)
{
    if (itf_fifo_full(sdspi->cmd_mst)) {
        return false;
    }
    sdspi_cmd_if_t pkt = {
        .kind = SDSPI_CMD_KIND_CONFIG,
        .enable = (sdspi->ctrl & SDSPI_CTRL_ENABLE) != 0,
        .clock_div = sdspi->clock_div
    };
    itf_write(sdspi->cmd_mst, &pkt);
    sdspi->config_pending = false;
    return true;
}

static bool sdspi_send_cmd(sdspi_t *sdspi)
{
    if (itf_fifo_full(sdspi->cmd_mst)) {
        return false;
    }
    sdspi_cmd_if_t pkt = {
        .kind = SDSPI_CMD_KIND_COMMAND,
        .opcode = sdspi->cmd_ctrl & SDSPI_CMD_CTRL_OPCODE_MASK,
        .arg = sdspi->cmd_arg,
        .rsp_type = (sdspi->cmd_ctrl & SDSPI_CMD_CTRL_RSP_MASK) >>
            SDSPI_CMD_CTRL_RSP_SHIFT,
        .data_present = sdspi->data_len != 0,
        .write = sdspi->data_write,
        .block_size = sdspi->block_size,
        .block_count = sdspi->block_count,
        .data_len = sdspi->data_len
    };
    itf_write(sdspi->cmd_mst, &pkt);
    sdspi->state = SDSPI_STATE_WAIT_CMD_RSP;
    return true;
}

static bool sdspi_send_write_data(sdspi_t *sdspi)
{
    if (sdspi->fifo_count == 0 || itf_fifo_full(sdspi->cmd_mst)) {
        return false;
    }
    sdspi->protocol_offset += sizeof(u32);
    sdspi_cmd_if_t pkt = {
        .kind = SDSPI_CMD_KIND_WRITE_DATA,
        .data = sdspi_fifo_front(sdspi),
        .last = sdspi->protocol_offset == sdspi->data_len
    };
    itf_write(sdspi->cmd_mst, &pkt);
    sdspi_fifo_pop(sdspi);
    if (pkt.last) {
        sdspi->state = SDSPI_STATE_WAIT_WRITE_DONE;
    }
    return true;
}

static void sdspi_protocol_tx(sdspi_t *sdspi)
{
    if (sdspi->reset_pending) {
        (void)sdspi_send_reset(sdspi);
        return;
    }
    if (sdspi->config_pending) {
        (void)sdspi_send_config(sdspi);
        return;
    }
    if (sdspi->state == SDSPI_STATE_SEND_CMD) {
        (void)sdspi_send_cmd(sdspi);
    } else if (sdspi->state == SDSPI_STATE_WRITE_STREAM) {
        (void)sdspi_send_write_data(sdspi);
    }
}

static void sdspi_update_card_status(sdspi_t *sdspi,
    const sdspi_data_if_t *pkt)
{
    bool changed = sdspi->card_status_valid &&
        (sdspi->card_present != pkt->card_present ||
         sdspi->read_only != pkt->read_only);
    sdspi->card_present = pkt->card_present;
    sdspi->read_only = pkt->read_only;
    sdspi->card_idle = pkt->card_idle;
    sdspi->card_status_valid = true;
    if (changed) {
        sdspi->irq_status |= SDSPI_IRQ_CARD_CHANGE;
        sdspi_update_irq(sdspi);
    }
}

static void sdspi_protocol_error(sdspi_t *sdspi)
{
    if (sdspi->state != SDSPI_STATE_IDLE) {
        sdspi->reset_pending = true;
        sdspi_finish(sdspi, sdspi->data_len != 0, true, 0,
            SDSPI_DATA_STATUS_IO_ERROR);
    }
}

static void sdspi_recv_data(sdspi_t *sdspi)
{
    if (itf_fifo_empty(sdspi->data_slv)) {
        return;
    }

    sdspi_data_if_t pkt;
    itf_fifo_get_front(sdspi->data_slv, &pkt);
    if (pkt.kind == SDSPI_DATA_KIND_READ_DATA &&
        sdspi->fifo_count == SDSPI_STREAM_FIFO_WORDS) {
        return;
    }
    itf_read(sdspi->data_slv, &pkt);
    sdspi_update_card_status(sdspi, &pkt);
    if (pkt.kind == SDSPI_DATA_KIND_STATUS) {
        return;
    }

    if (pkt.kind == SDSPI_DATA_KIND_RESPONSE) {
        if (sdspi->state != SDSPI_STATE_WAIT_CMD_RSP) {
            sdspi_protocol_error(sdspi);
            return;
        }
        sdspi->resp0 = pkt.resp0;
        sdspi->resp1 = pkt.resp1;
        if (pkt.error) {
            sdspi_finish(sdspi, sdspi->data_len != 0, true,
                pkt.cmd_error, pkt.data_error);
        } else if (sdspi->data_len == 0) {
            sdspi_finish(sdspi, false, false, 0, 0);
        } else if (sdspi->data_write) {
            sdspi->state = SDSPI_STATE_WRITE_STREAM;
        } else {
            sdspi->state = SDSPI_STATE_READ_STREAM;
        }
        return;
    }

    if (pkt.kind == SDSPI_DATA_KIND_READ_DATA) {
        if (sdspi->state != SDSPI_STATE_READ_STREAM ||
            sdspi->protocol_offset + sizeof(u32) > sdspi->data_len) {
            sdspi_protocol_error(sdspi);
            return;
        }
        sdspi_fifo_push(sdspi, pkt.data);
        sdspi->protocol_offset += sizeof(pkt.data);
        bool expected_last = sdspi->protocol_offset == sdspi->data_len;
        if (pkt.last != expected_last) {
            sdspi_protocol_error(sdspi);
            return;
        }
        sdspi->read_input_done = pkt.last;
        return;
    }

    if (pkt.kind == SDSPI_DATA_KIND_WRITE_DONE) {
        if (sdspi->state != SDSPI_STATE_WAIT_WRITE_DONE ||
            sdspi->protocol_offset != sdspi->data_len ||
            sdspi->dma_offset != sdspi->data_len ||
            sdspi->dma_state != SDSPI_DMA_IDLE || sdspi->fifo_count != 0) {
            sdspi_protocol_error(sdspi);
            return;
        }
        sdspi_finish(sdspi, true, pkt.error, pkt.cmd_error,
            pkt.data_error);
        return;
    }

    sdspi_protocol_error(sdspi);
}

void sdspi_clock(sdspi_t *sdspi)
{
    mod_clock(&sdspi->mod);
    sdspi_recv_data(sdspi);
    sdspi_dma_proc(sdspi);
    sdspi_update_prefetch(sdspi);
    sdspi_dma_schedule(sdspi);
    sdspi_protocol_tx(sdspi);
    sdspi_apb_proc(sdspi);
}

void sdspi_free(sdspi_t *sdspi)
{
    mod_free(&sdspi->mod);
}
