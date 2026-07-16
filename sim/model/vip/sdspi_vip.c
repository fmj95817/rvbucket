#include "sdspi_vip.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "dbg/chk.h"
#include "dbg/vcd.h"

#define SD_R1_IDLE            (1u << 0)
#define SD_R1_ILLEGAL_COMMAND (1u << 2)
#define SD_R1_ADDRESS_ERROR   (1u << 5)
#define SD_R1_PARAMETER_ERROR (1u << 6)
#define SD_OCR                0xc0ff8000u

static u8 sdspi_vip_crc7(const u8 *data, u32 size)
{
    u8 crc = 0;
    for (u32 i = 0; i < size; i++) {
        for (s32 bit = 7; bit >= 0; bit--) {
            crc <<= 1;
            if (((crc >> 7) ^ ((data[i] >> bit) & 1u)) != 0) {
                crc ^= 0x89u;
            }
        }
    }
    return crc;
}

static void sdspi_vip_fill_status(const sdspi_vip_t *vip,
    sdspi_data_if_t *pkt)
{
    pkt->card_present = vip->card_present;
    pkt->read_only = vip->read_only;
    pkt->card_idle = vip->card_idle;
}

static u8 sdspi_vip_r1(const sdspi_vip_t *vip)
{
    return vip->card_idle ? SD_R1_IDLE : 0;
}

static void sdspi_vip_protocol_reset(sdspi_vip_t *vip)
{
    vip->card_idle = true;
    vip->app_cmd = false;
    vip->state = SDSPI_VIP_IDLE;
    vip->data_len = 0;
    vip->data_offset = 0;
    vip->image_offset = 0;
    vip->data_write = false;
    vip->status_pending = true;
    vip->enabled = false;
    vip->clock_div = 0;
    memset(&vip->result, 0, sizeof(vip->result));
}

void sdspi_vip_construct(sdspi_vip_t *vip, const char *parent,
    const char *name, const sdspi_vip_conf_t *conf)
{
    mod_construct(&vip->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);
    DBG_CHECK(conf != NULL);
    DBG_CHECK(vip->cmd_slv != NULL);
    DBG_CHECK(vip->data_mst != NULL);

    vip->image = NULL;
    vip->image_size = 0;
    vip->image_read_only = false;
    vip->card_present = false;
    vip->read_only = false;
    vip->data_buf = malloc(SDSPI_MAX_DATA_SIZE);
    DBG_CHECK(vip->data_buf != NULL);

    if (conf->image_path != NULL) {
        vip->image = fopen(conf->image_path, "r+b");
        if (vip->image == NULL) {
            vip->image = fopen(conf->image_path, "rb");
            vip->image_read_only = vip->image != NULL;
        }
        if (vip->image == NULL) {
            fprintf(stderr, "sdspi_vip: cannot open '%s': %s\n",
                conf->image_path, strerror(errno));
        }
        DBG_CHECK(vip->image != NULL);
        DBG_CHECK(fseeko(vip->image, 0, SEEK_END) == 0);
        off_t image_size = ftello(vip->image);
        DBG_CHECK(image_size >= 0);
        DBG_CHECK(fseeko(vip->image, 0, SEEK_SET) == 0);
        vip->image_size = (u64)image_size;
        vip->card_present = vip->image_size >= SDSPI_SECTOR_SIZE;
        vip->read_only = vip->image_read_only;
    }

    sdspi_vip_protocol_reset(vip);
    dbg_vcd_add_sig("state", DBG_SIG_TYPE_REG, 32, &vip->state);
    dbg_vcd_add_sig("card_present", DBG_SIG_TYPE_REG, 1,
        &vip->card_present);
}

void sdspi_vip_reset(sdspi_vip_t *vip)
{
    mod_reset(&vip->mod);
    vip->card_present = vip->image != NULL &&
        vip->image_size >= SDSPI_SECTOR_SIZE;
    vip->read_only = vip->image_read_only;
    sdspi_vip_protocol_reset(vip);
}

void sdspi_vip_set_card_present(sdspi_vip_t *vip, bool present)
{
    bool new_present = present && vip->image != NULL &&
        vip->image_size >= SDSPI_SECTOR_SIZE;
    if (vip->card_present == new_present) {
        return;
    }
    vip->card_present = new_present;
    vip->read_only = new_present && vip->image_read_only;
    if (!new_present) {
        vip->card_idle = true;
        vip->app_cmd = false;
    }
    vip->status_pending = true;
}

static void sdspi_vip_make_cid(sdspi_vip_t *vip)
{
    static const u8 prefix[9] = {
        0x52, 'R', 'V', 'R', 'V', 'B', 'S', 'D', 0x10
    };
    memcpy(vip->data_buf, prefix, sizeof(prefix));
    vip->data_buf[9] = 0x12;
    vip->data_buf[10] = 0x34;
    vip->data_buf[11] = 0x56;
    vip->data_buf[12] = 0x78;
    vip->data_buf[13] = 0x02;
    vip->data_buf[14] = 0x67;
    vip->data_buf[15] = (sdspi_vip_crc7(vip->data_buf, 15) << 1) | 1u;
}

static void sdspi_vip_make_csd(sdspi_vip_t *vip)
{
    u64 units = vip->image_size / (512u * 1024u);
    u32 csize = units == 0 ? 0 : (u32)(units - 1u);
    if (csize > 0x3fffffu) {
        csize = 0x3fffffu;
    }

    u8 *csd = vip->data_buf;
    csd[0] = 0x40;
    csd[1] = 0x0e;
    csd[2] = 0x00;
    csd[3] = 0x32;
    csd[4] = 0x5b;
    csd[5] = 0x59;
    csd[6] = 0x00;
    csd[7] = (u8)(csize >> 16u);
    csd[8] = (u8)(csize >> 8u);
    csd[9] = (u8)csize;
    csd[10] = 0x7f;
    csd[11] = 0x80;
    csd[12] = 0x0a;
    csd[13] = 0x40;
    csd[14] = 0x00;
    csd[15] = (sdspi_vip_crc7(csd, 15) << 1) | 1u;
}

static bool sdspi_vip_prepare_regular_data(sdspi_vip_t *vip,
    const sdspi_cmd_if_t *cmd, bool write)
{
    if (cmd->block_size == 0 || cmd->block_count == 0) {
        return false;
    }
    u64 length = (u64)cmd->block_size * cmd->block_count;
    if (length != cmd->data_len || length > SDSPI_MAX_DATA_SIZE) {
        return false;
    }
    if ((cmd->opcode == 17u || cmd->opcode == 24u) &&
        cmd->block_count != 1u) {
        return false;
    }

    vip->image_offset = (u64)cmd->arg * SDSPI_SECTOR_SIZE;
    if (vip->image_offset > vip->image_size ||
        length > vip->image_size - vip->image_offset) {
        return false;
    }
    if (write && vip->read_only) {
        return false;
    }
    return true;
}

static bool sdspi_vip_prepare_acmd(sdspi_vip_t *vip,
    const sdspi_cmd_if_t *cmd, u32 *cmd_error)
{
    switch (cmd->opcode) {
    case 13: /* SD_STATUS */
        if (vip->data_len != 64u || vip->data_write) {
            *cmd_error = SDSPI_CMD_STATUS_PARAMETER;
            return false;
        }
        memset(vip->data_buf, 0, 64);
        return true;
    case 23: /* SET_WR_BLK_ERASE_COUNT */
        return vip->data_len == 0;
    case 41: /* SD_SEND_OP_COND */
        vip->card_idle = false;
        return vip->data_len == 0;
    case 51: /* SEND_SCR */
        if (vip->data_len != 8u || vip->data_write) {
            *cmd_error = SDSPI_CMD_STATUS_PARAMETER;
            return false;
        }
        memset(vip->data_buf, 0, 8);
        vip->data_buf[0] = 0x02;
        vip->data_buf[1] = 0x25;
        return true;
    default:
        *cmd_error = SDSPI_CMD_STATUS_ILLEGAL;
        vip->result.resp0 |= SD_R1_ILLEGAL_COMMAND;
        return false;
    }
}

static bool sdspi_vip_prepare_cmd(sdspi_vip_t *vip,
    const sdspi_cmd_if_t *cmd, u32 *cmd_error)
{
    switch (cmd->opcode) {
    case 0: /* GO_IDLE_STATE */
        vip->card_idle = true;
        vip->app_cmd = false;
        return vip->data_len == 0;
    case 6: /* SWITCH_FUNC */
        if (vip->data_len != 64u || vip->data_write) {
            *cmd_error = SDSPI_CMD_STATUS_PARAMETER;
            return false;
        }
        memset(vip->data_buf, 0, 64);
        return true;
    case 8: /* SEND_IF_COND */
        vip->result.resp1 = cmd->arg;
        return vip->data_len == 0;
    case 9: /* SEND_CSD */
        if (vip->data_len != 16u || vip->data_write) {
            *cmd_error = SDSPI_CMD_STATUS_PARAMETER;
            return false;
        }
        sdspi_vip_make_csd(vip);
        return true;
    case 10: /* SEND_CID */
        if (vip->data_len != 16u || vip->data_write) {
            *cmd_error = SDSPI_CMD_STATUS_PARAMETER;
            return false;
        }
        sdspi_vip_make_cid(vip);
        return true;
    case 12: /* STOP_TRANSMISSION */
    case 13: /* SEND_STATUS */
    case 23: /* SET_BLOCK_COUNT */
        return vip->data_len == 0;
    case 16: /* SET_BLOCKLEN */
        if (cmd->arg == 0 || cmd->arg > SDSPI_SECTOR_SIZE) {
            *cmd_error = SDSPI_CMD_STATUS_PARAMETER;
            vip->result.resp0 |= SD_R1_PARAMETER_ERROR;
            return false;
        }
        return vip->data_len == 0;
    case 17: /* READ_SINGLE_BLOCK */
    case 18: /* READ_MULTIPLE_BLOCK */
        if (vip->data_write ||
            !sdspi_vip_prepare_regular_data(vip, cmd, false)) {
            *cmd_error = SDSPI_CMD_STATUS_ADDRESS;
            vip->result.resp0 |= SD_R1_ADDRESS_ERROR;
            return false;
        }
        return true;
    case 24: /* WRITE_BLOCK */
    case 25: /* WRITE_MULTIPLE_BLOCK */
        if (!vip->data_write ||
            !sdspi_vip_prepare_regular_data(vip, cmd, true)) {
            *cmd_error = SDSPI_CMD_STATUS_ADDRESS;
            vip->result.resp0 |= SD_R1_ADDRESS_ERROR;
            return false;
        }
        return true;
    case 55: /* APP_CMD */
        vip->app_cmd = true;
        return vip->data_len == 0;
    case 58: /* READ_OCR */
        vip->result.resp1 = SD_OCR;
        return vip->data_len == 0;
    case 59: /* CRC_ON_OFF */
        return vip->data_len == 0;
    default:
        *cmd_error = SDSPI_CMD_STATUS_ILLEGAL;
        vip->result.resp0 |= SD_R1_ILLEGAL_COMMAND;
        return false;
    }
}

static void sdspi_vip_prepare_response(sdspi_vip_t *vip,
    const sdspi_cmd_if_t *cmd)
{
    memset(&vip->result, 0, sizeof(vip->result));
    vip->result.kind = SDSPI_DATA_KIND_RESPONSE;
    vip->data_len = cmd->data_present ? cmd->data_len : 0;
    vip->data_write = cmd->data_present && cmd->write;
    vip->data_offset = 0;

    u32 cmd_error = 0;
    bool valid_shape = (!cmd->data_present && cmd->data_len == 0 &&
        !cmd->write) || (cmd->data_present && cmd->data_len != 0 &&
        cmd->data_len <= SDSPI_MAX_DATA_SIZE &&
        (cmd->data_len & 3u) == 0);
    if (!vip->enabled || !vip->card_present) {
        cmd_error = SDSPI_CMD_STATUS_TIMEOUT;
    } else if (!valid_shape) {
        cmd_error = SDSPI_CMD_STATUS_PARAMETER;
    } else {
        bool was_app_cmd = vip->app_cmd && cmd->opcode != 55u;
        if (was_app_cmd) {
            vip->app_cmd = false;
        }
        vip->result.resp0 = sdspi_vip_r1(vip);
        bool ok = was_app_cmd ? sdspi_vip_prepare_acmd(vip, cmd,
            &cmd_error) : sdspi_vip_prepare_cmd(vip, cmd, &cmd_error);
        vip->result.resp0 = (vip->result.resp0 & ~SD_R1_IDLE) |
            sdspi_vip_r1(vip);
        if (!ok && cmd_error == 0) {
            cmd_error = SDSPI_CMD_STATUS_PARAMETER;
        }
    }

    if (cmd_error == 0 && vip->data_len != 0 && !vip->data_write &&
        (cmd->opcode == 17u || cmd->opcode == 18u)) {
        ssize_t size = pread(fileno(vip->image), vip->data_buf,
            vip->data_len, (off_t)vip->image_offset);
        if (size != (ssize_t)vip->data_len) {
            vip->result.data_error = SDSPI_DATA_STATUS_IO_ERROR;
        }
    }

    vip->result.cmd_error = cmd_error;
    vip->result.error = cmd_error != 0 || vip->result.data_error != 0;
    sdspi_vip_fill_status(vip, &vip->result);
    vip->state = SDSPI_VIP_SEND_RESPONSE;
}

static void sdspi_vip_recv_cmd(sdspi_vip_t *vip)
{
    if (itf_fifo_empty(vip->cmd_slv)) {
        return;
    }

    sdspi_cmd_if_t pkt;
    itf_read(vip->cmd_slv, &pkt);
    if (pkt.kind == SDSPI_CMD_KIND_RESET) {
        sdspi_vip_protocol_reset(vip);
        return;
    }
    if (pkt.kind == SDSPI_CMD_KIND_CONFIG) {
        if (vip->state != SDSPI_VIP_IDLE) {
            memset(&vip->result, 0, sizeof(vip->result));
            vip->result.kind = SDSPI_DATA_KIND_WRITE_DONE;
            vip->result.error = true;
            vip->result.data_error = SDSPI_DATA_STATUS_IO_ERROR;
            sdspi_vip_fill_status(vip, &vip->result);
            vip->state = SDSPI_VIP_SEND_WRITE_DONE;
            return;
        }
        vip->enabled = pkt.enable;
        vip->clock_div = pkt.clock_div;
        return;
    }
    if (vip->state == SDSPI_VIP_IDLE &&
        pkt.kind == SDSPI_CMD_KIND_COMMAND) {
        sdspi_vip_prepare_response(vip, &pkt);
        return;
    }
    if (vip->state != SDSPI_VIP_RECV_WRITE_DATA ||
        pkt.kind != SDSPI_CMD_KIND_WRITE_DATA ||
        vip->data_offset + sizeof(u32) > vip->data_len) {
        memset(&vip->result, 0, sizeof(vip->result));
        vip->result.kind = SDSPI_DATA_KIND_WRITE_DONE;
        vip->result.error = true;
        vip->result.data_error = SDSPI_DATA_STATUS_IO_ERROR;
        sdspi_vip_fill_status(vip, &vip->result);
        vip->state = SDSPI_VIP_SEND_WRITE_DONE;
        return;
    }

    memcpy(vip->data_buf + vip->data_offset, &pkt.data, sizeof(pkt.data));
    vip->data_offset += sizeof(pkt.data);
    bool expected_last = vip->data_offset == vip->data_len;
    if (pkt.last != expected_last) {
        memset(&vip->result, 0, sizeof(vip->result));
        vip->result.kind = SDSPI_DATA_KIND_WRITE_DONE;
        vip->result.error = true;
        vip->result.data_error = SDSPI_DATA_STATUS_IO_ERROR;
        sdspi_vip_fill_status(vip, &vip->result);
        vip->state = SDSPI_VIP_SEND_WRITE_DONE;
        return;
    }
    if (!pkt.last) {
        return;
    }

    ssize_t size = pwrite(fileno(vip->image), vip->data_buf, vip->data_len,
        (off_t)vip->image_offset);
    memset(&vip->result, 0, sizeof(vip->result));
    vip->result.kind = SDSPI_DATA_KIND_WRITE_DONE;
    if (size != (ssize_t)vip->data_len) {
        vip->result.error = true;
        vip->result.data_error = SDSPI_DATA_STATUS_IO_ERROR;
    }
    sdspi_vip_fill_status(vip, &vip->result);
    vip->state = SDSPI_VIP_SEND_WRITE_DONE;
}

static bool sdspi_vip_send_status(sdspi_vip_t *vip)
{
    if (!vip->status_pending || itf_fifo_full(vip->data_mst)) {
        return false;
    }
    sdspi_data_if_t pkt = { .kind = SDSPI_DATA_KIND_STATUS };
    sdspi_vip_fill_status(vip, &pkt);
    itf_write(vip->data_mst, &pkt);
    vip->status_pending = false;
    return true;
}

static void sdspi_vip_send_response(sdspi_vip_t *vip)
{
    if (itf_fifo_full(vip->data_mst)) {
        return;
    }
    itf_write(vip->data_mst, &vip->result);
    if (vip->result.error || vip->data_len == 0) {
        vip->state = SDSPI_VIP_IDLE;
    } else if (vip->data_write) {
        vip->state = SDSPI_VIP_RECV_WRITE_DATA;
    } else {
        vip->state = SDSPI_VIP_SEND_READ_DATA;
    }
}

static void sdspi_vip_send_read_data(sdspi_vip_t *vip)
{
    if (itf_fifo_full(vip->data_mst)) {
        return;
    }
    sdspi_data_if_t pkt = { .kind = SDSPI_DATA_KIND_READ_DATA };
    memcpy(&pkt.data, vip->data_buf + vip->data_offset, sizeof(pkt.data));
    vip->data_offset += sizeof(pkt.data);
    pkt.last = vip->data_offset == vip->data_len;
    sdspi_vip_fill_status(vip, &pkt);
    itf_write(vip->data_mst, &pkt);
    if (pkt.last) {
        vip->state = SDSPI_VIP_IDLE;
    }
}

void sdspi_vip_clock(sdspi_vip_t *vip)
{
    mod_clock(&vip->mod);
    if (sdspi_vip_send_status(vip)) {
        return;
    }

    switch (vip->state) {
    case SDSPI_VIP_IDLE:
    case SDSPI_VIP_RECV_WRITE_DATA:
        sdspi_vip_recv_cmd(vip);
        break;
    case SDSPI_VIP_SEND_RESPONSE:
        sdspi_vip_send_response(vip);
        break;
    case SDSPI_VIP_SEND_READ_DATA:
        sdspi_vip_send_read_data(vip);
        break;
    case SDSPI_VIP_SEND_WRITE_DONE:
        if (!itf_fifo_full(vip->data_mst)) {
            itf_write(vip->data_mst, &vip->result);
            vip->state = SDSPI_VIP_IDLE;
        }
        break;
    }
}

void sdspi_vip_free(sdspi_vip_t *vip)
{
    mod_free(&vip->mod);
    if (vip->image != NULL) {
        fclose(vip->image);
    }
    free(vip->data_buf);
}
