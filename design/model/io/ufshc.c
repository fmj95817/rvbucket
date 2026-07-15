#include "io/ufshc.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "dbg/env.h"
#include "dbg/chk.h"
#include "dbg/log.h"
#include "dbg/vcd.h"

#define UFSHC_LOG(u, fmt, ...) \
do { \
    if ((u)->log_enable) { \
        u64 cycle = ((u)->mod.cycle != NULL) ? *((u)->mod.cycle) : 0; \
        DBG_LOG(LOG_PRINT, "ufshc[%llu]: " fmt, \
                (unsigned long long)cycle, ##__VA_ARGS__); \
    } \
} while (0)

static inline u16 bswap16(u16 v)
{
    return (u16)((v >> 8) | (v << 8));
}

static inline u32 bswap32(u32 v)
{
    return ((v >> 24) & 0x000000ffu) |
           ((v >> 8)  & 0x0000ff00u) |
           ((v << 8)  & 0x00ff0000u) |
           ((v << 24) & 0xff000000u);
}

static inline u64 bswap64(u64 v)
{
    return ((u64)bswap32((u32)v) << 32) | bswap32((u32)(v >> 32));
}

static inline u16 be16_to_cpu(u16 v) { return bswap16(v); }
static inline u32 be32_to_cpu(u32 v) { return bswap32(v); }
static inline u64 be64_to_cpu(u64 v) { return bswap64(v); }
static inline u16 cpu_to_be16(u16 v) { return bswap16(v); }
static inline u32 cpu_to_be32(u32 v) { return bswap32(v); }
static inline u64 cpu_to_be64(u64 v) { return bswap64(v); }
static inline u16 le16_to_cpu(u16 v) { return v; }
static inline u32 le32_to_cpu(u32 v) { return v; }
static inline u64 le64_to_cpu(u64 v) { return v; }
static inline u16 cpu_to_le16(u16 v) { return v; }
static inline u32 cpu_to_le32(u32 v) { return v; }
static inline u64 cpu_to_le64(u64 v) { return v; }

static inline u32 load_be32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

static inline u64 load_be64(const u8 *p)
{
    return ((u64)load_be32(p) << 32) | load_be32(p + 4);
}

static inline void store_be16(u8 *p, u16 v)
{
    p[0] = (u8)(v >> 8);
    p[1] = (u8)v;
}

static inline void store_be32(u8 *p, u32 v)
{
    p[0] = (u8)(v >> 24);
    p[1] = (u8)(v >> 16);
    p[2] = (u8)(v >> 8);
    p[3] = (u8)v;
}

static inline void store_be64(u8 *p, u64 v)
{
    store_be32(p, (u32)(v >> 32));
    store_be32(p + 4, (u32)v);
}

static u64 ufshc_file_size(FILE *fp)
{
    DBG_CHECK(fp != NULL);
    DBG_CHECK(fseek(fp, 0, SEEK_END) == 0);
    long size = ftell(fp);
    DBG_CHECK(size >= 0);
    DBG_CHECK(fseek(fp, 0, SEEK_SET) == 0);
    return (u64)size;
}

static bool ufshc_file_is_empty(FILE *fp)
{
    return ufshc_file_size(fp) == 0;
}

static bool ufshc_file_extend(FILE *fp, u64 size)
{
    DBG_CHECK(fp != NULL);
    if (size == 0) {
        return true;
    }
    if (fseek(fp, (long)(size - 1u), SEEK_SET) != 0) {
        return false;
    }
    if (fputc(0, fp) == EOF) {
        return false;
    }
    return fflush(fp) == 0;
}

static void ufshc_make_image_header(ufshc_image_header_t *hdr, u32 storage_size)
{
    memset(hdr, 0, sizeof(*hdr));
    memcpy(hdr->magic, UFSHC_IMAGE_MAGIC, UFSHC_IMAGE_MAGIC_SIZE);
    hdr->version = cpu_to_le32(UFSHC_IMAGE_VERSION);
    hdr->header_size = cpu_to_le32(UFSHC_IMAGE_HEADER_SIZE);
    hdr->storage_size = cpu_to_le64(storage_size);
    hdr->block_size = cpu_to_le32(UFSHC_BLOCK_SIZE);
}

static bool ufshc_header_valid(const ufshc_image_header_t *hdr)
{
    return memcmp(hdr->magic, UFSHC_IMAGE_MAGIC, UFSHC_IMAGE_MAGIC_SIZE) == 0 &&
           le32_to_cpu(hdr->version) == UFSHC_IMAGE_VERSION &&
           le32_to_cpu(hdr->header_size) == UFSHC_IMAGE_HEADER_SIZE &&
           le32_to_cpu(hdr->block_size) == UFSHC_BLOCK_SIZE &&
           le64_to_cpu(hdr->storage_size) <= 0xffffffffu &&
           (le64_to_cpu(hdr->storage_size) & (UFSHC_BLOCK_SIZE - 1u)) == 0;
}

static bool ufshc_write_image_header(FILE *fp, u32 storage_size)
{
    ufshc_image_header_t hdr;
    ufshc_make_image_header(&hdr, storage_size);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        return false;
    }
    if (fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) {
        return false;
    }
    return fflush(fp) == 0;
}

static u32 ufshc_default_backing_size(void)
{
    return UFSHC_STORAGE_SIZE & ~(UFSHC_BLOCK_SIZE - 1u);
}

static inline u32 mcq_reg_addr(u32 qid)
{
    return UFSHC_MCQ_REG_BASE + qid * UFSHC_MCQ_REG_STRIDE;
}

static inline u32 mcq_op_addr(u32 qid)
{
    return UFSHC_MCQ_OP_BASE + qid * UFSHC_MCQ_OP_STRIDE;
}

static void ufshc_irq_check(ufshc_t *u)
{
    bool irq = ((u->is & UFSHC_INTR_MASK) & u->ie) != 0;
    if (u->irq_o != NULL && u->irq_o->irq != irq) {
        UFSHC_LOG(u, "irq %u is=%08x ie=%08x\n", irq, u->is, u->ie);
        u->irq_o->irq = irq;
        itf_signal_write_notify(u->irq_out);
    }
}

static void ufshc_init_descs(ufshc_t *u)
{
    memset(u->device_desc, 0, sizeof(u->device_desc));
    u->device_desc[0] = sizeof(u->device_desc);
    u->device_desc[1] = UFSHC_DESC_IDN_DEVICE;
    u->device_desc[4] = 0x01;
    u->device_desc[6] = 0x01;
    u->device_desc[7] = 0x04;
    u->device_desc[10] = 0x01;
    u->device_desc[11] = 0x7f;
    store_be16(&u->device_desc[0x10], UFSHC_SPEC_VER);
    u->device_desc[0x14] = 0x00;
    u->device_desc[0x15] = 0x01;
    u->device_desc[0x16] = 0x02;
    u->device_desc[0x17] = 0x03;
    store_be16(&u->device_desc[0x18], 0x1234);
    u->device_desc[0x1a] = 0x16;
    u->device_desc[0x1b] = 0x1a;
    u->device_desc[0x1c] = 0x02;
    u->device_desc[0x21] = UFSHC_MAX_QNUM;
    u->device_desc[0x2a] = 0x04;

    memset(u->unit_desc, 0, sizeof(u->unit_desc));
    u->unit_desc[0] = sizeof(u->unit_desc);
    u->unit_desc[1] = UFSHC_DESC_IDN_UNIT;
    u->unit_desc[2] = 0;
    u->unit_desc[3] = 1;
    u->unit_desc[6] = UFSHC_MAX_QNUM;
    u->unit_desc[0x0a] = UFSHC_BLOCK_SHIFT;
    store_be64(&u->unit_desc[0x0b], u->storage_size >> UFSHC_BLOCK_SHIFT);

    memset(u->geometry_desc, 0, sizeof(u->geometry_desc));
    u->geometry_desc[0] = sizeof(u->geometry_desc);
    u->geometry_desc[1] = UFSHC_DESC_IDN_GEOMETRY;
    store_be64(&u->geometry_desc[0x04], u->storage_size >> 9);
    u->geometry_desc[0x0c] = 0x01;
    store_be32(&u->geometry_desc[0x0d], 0x2000);
    u->geometry_desc[0x11] = 0x01;
    u->geometry_desc[0x12] = 0x08;
    u->geometry_desc[0x15] = 0x08;
    u->geometry_desc[0x16] = 0x08;
    u->geometry_desc[0x17] = 0x40;
    u->geometry_desc[0x1a] = 0x05;
    store_be16(&u->geometry_desc[0x1e], 0x8001);

    memset(u->flags, 0, sizeof(u->flags));
    u->flags[UFSHC_FLAG_IDN_PERM_FW_DISABLE] = 1;

    memset(u->attrs, 0, sizeof(u->attrs));
    u->attrs[UFSHC_ATTR_IDN_POWER_MODE] = 1;
    u->attrs[UFSHC_ATTR_IDN_MAX_DATA_IN] = 0x08;
    u->attrs[UFSHC_ATTR_IDN_MAX_DATA_OUT] = 0x08;
    u->attrs[UFSHC_ATTR_IDN_REF_CLK_FREQ] = 0x01;
    u->attrs[UFSHC_ATTR_IDN_CONF_DESC_LOCK] = 0x01;
    u->attrs[UFSHC_ATTR_IDN_MAX_NUM_RTT] = 0x02;
    u->attrs[UFSHC_ATTR_IDN_REF_CLK_WAIT] = 0xff;
    u->attrs[UFSHC_ATTR_IDN_HIGH_TEMP_BOUND] = 160;
    u->attrs[UFSHC_ATTR_IDN_LOW_TEMP_BOUND] = 60;
}

static bool ufshc_open_backing(ufshc_t *u)
{
    u->storage_size = 0;
    DBG_CHECK(u->backing_path != NULL && u->backing_path[0] != '\0');

    u->backing_fp = fopen(u->backing_path, "r+b");
    if (u->backing_fp == NULL) {
        u->backing_fp = fopen(u->backing_path, "w+b");
    }
    if (u->backing_fp == NULL) {
        UFSHC_LOG(u, "failed to open backing file %s: %s\n",
                  u->backing_path, strerror(errno));
        return false;
    }

    if (ufshc_file_is_empty(u->backing_fp)) {
        u32 size = ufshc_default_backing_size();
        if (size == 0 ||
            !ufshc_write_image_header(u->backing_fp, size) ||
            !ufshc_file_extend(u->backing_fp, UFSHC_IMAGE_HEADER_SIZE + (u64)size)) {
            UFSHC_LOG(u, "failed to create backing file %s\n", u->backing_path);
            return false;
        }
        u->storage_size = size;
        UFSHC_LOG(u, "created backing file %s size=%u\n",
                  u->backing_path, u->storage_size);
        return true;
    }

    ufshc_image_header_t hdr;
    if (fseek(u->backing_fp, 0, SEEK_SET) != 0 ||
        fread(&hdr, 1, sizeof(hdr), u->backing_fp) != sizeof(hdr) ||
        !ufshc_header_valid(&hdr)) {
        UFSHC_LOG(u, "invalid backing file header %s\n", u->backing_path);
        return false;
    }

    u->storage_size = (u32)le64_to_cpu(hdr.storage_size);
    u64 required_size = UFSHC_IMAGE_HEADER_SIZE + (u64)u->storage_size;
    if (ufshc_file_size(u->backing_fp) < required_size &&
        !ufshc_file_extend(u->backing_fp, required_size)) {
        UFSHC_LOG(u, "failed to extend backing file %s\n", u->backing_path);
        return false;
    }
    UFSHC_LOG(u, "opened backing file %s size=%u\n",
              u->backing_path, u->storage_size);
    return true;
}

static bool ufshc_backend_read(ufshc_t *u, u32 off, u8 *buf, u32 len)
{
    if (len == 0) {
        return true;
    }
    if (u->backing_fp == NULL || off > u->storage_size ||
        len > u->storage_size - off) {
        return false;
    }
    if (fseek(u->backing_fp, (long)(UFSHC_IMAGE_HEADER_SIZE + off), SEEK_SET) != 0) {
        return false;
    }
    return fread(buf, 1, len, u->backing_fp) == len;
}

static bool ufshc_backend_write(ufshc_t *u, u32 off, const u8 *buf, u32 len)
{
    if (len == 0) {
        return true;
    }
    if (u->backing_fp == NULL || off > u->storage_size ||
        len > u->storage_size - off) {
        return false;
    }
    if (fseek(u->backing_fp, (long)(UFSHC_IMAGE_HEADER_SIZE + off), SEEK_SET) != 0) {
        return false;
    }
    if (fwrite(buf, 1, len, u->backing_fp) != len) {
        return false;
    }
    return fflush(u->backing_fp) == 0;
}

static void ufshc_dma_start(ufshc_t *u, bool write, u32 addr, u8 *buf, u32 len)
{
    DBG_CHECK(u->dma.state == UFSHC_DMA_IDLE);
    u->dma.write = write;
    u->dma.ok = true;
    u->dma.addr = addr;
    u->dma.buf = buf;
    u->dma.len = len;
    u->dma.pos = 0;
    u->dma.state = write ? UFSHC_DMA_WR_SEND : UFSHC_DMA_RD_SEND;
    if (len == 0) {
        u->dma.state = UFSHC_DMA_IDLE;
    }
}

static bool ufshc_dma_done(ufshc_t *u)
{
    return u->dma.state == UFSHC_DMA_IDLE;
}

static u8 ufshc_dma_choose_beat(ufshc_dma_t *dma)
{
    u32 addr = dma->addr + dma->pos;
    u32 remain = dma->len - dma->pos;
    if ((addr & 3u) == 0 && remain >= 4) {
        return 4;
    }
    if ((addr & 1u) == 0 && remain >= 2) {
        return 2;
    }
    return 1;
}

static void ufshc_dma_clock(ufshc_t *u)
{
    ufshc_dma_t *d = &u->dma;

    switch (d->state) {
    case UFSHC_DMA_IDLE:
        break;
    case UFSHC_DMA_RD_SEND:
        if (itf_fifo_full(u->dma_axi4_ar_mst)) {
            break;
        }
        d->beat_addr = d->addr + d->pos;
        d->beat_bytes = ufshc_dma_choose_beat(d);
        d->beat_size = (d->beat_bytes == 4) ? AXI4_AR_SIZE_B4 :
                       (d->beat_bytes == 2) ? AXI4_AR_SIZE_B2 :
                                               AXI4_AR_SIZE_B1;
        {
            axi4_ar_if_t ar = {
                .id = UFSHC_AXI_ID,
                .addr = d->beat_addr,
                .len = 0,
                .size = d->beat_size,
                .burst = AXI4_AR_BURST_INCR,
                .lock = false,
                .cache = 0,
                .prot = 0,
                .qos = 0,
                .user = 0
            };
            itf_write(u->dma_axi4_ar_mst, &ar);
        }
        d->state = UFSHC_DMA_RD_WAIT;
        break;
    case UFSHC_DMA_RD_WAIT:
        if (itf_fifo_empty(u->dma_axi4_r_slv)) {
            break;
        }
        {
            axi4_r_if_t r;
            itf_read(u->dma_axi4_r_slv, &r);
            if (r.resp != AXI4_R_RESP_OKAY) {
                d->ok = false;
            }
            memcpy(d->buf + d->pos, &r.data, d->beat_bytes);
            d->pos += d->beat_bytes;
        }
        d->state = (d->pos >= d->len) ? UFSHC_DMA_IDLE : UFSHC_DMA_RD_SEND;
        break;
    case UFSHC_DMA_WR_SEND:
        if (itf_fifo_full(u->dma_axi4_aw_mst) ||
            itf_fifo_full(u->dma_axi4_w_mst)) {
            break;
        }
        d->beat_addr = d->addr + d->pos;
        d->beat_bytes = ufshc_dma_choose_beat(d);
        d->beat_size = (d->beat_bytes == 4) ? AXI4_AW_SIZE_B4 :
                       (d->beat_bytes == 2) ? AXI4_AW_SIZE_B2 :
                                               AXI4_AW_SIZE_B1;
        {
            u32 data = 0;
            memcpy(&data, d->buf + d->pos, d->beat_bytes);
            axi4_aw_if_t aw = {
                .id = UFSHC_AXI_ID,
                .addr = d->beat_addr,
                .len = 0,
                .size = d->beat_size,
                .burst = AXI4_AW_BURST_INCR,
                .lock = false,
                .cache = 0,
                .prot = 0,
                .qos = 0,
                .user = 0
            };
            axi4_w_if_t w = {
                .data = data,
                .strb = (u8)((1u << d->beat_bytes) - 1u),
                .last = true
            };
            itf_write(u->dma_axi4_aw_mst, &aw);
            itf_write(u->dma_axi4_w_mst, &w);
        }
        d->state = UFSHC_DMA_WR_WAIT;
        break;
    case UFSHC_DMA_WR_WAIT:
        if (itf_fifo_empty(u->dma_axi4_b_slv)) {
            break;
        }
        {
            axi4_b_if_t b;
            itf_read(u->dma_axi4_b_slv, &b);
            if (b.resp != AXI4_B_RESP_OKAY) {
                d->ok = false;
            }
            d->pos += d->beat_bytes;
        }
        d->state = (d->pos >= d->len) ? UFSHC_DMA_IDLE : UFSHC_DMA_WR_SEND;
        break;
    }
}

static bool ufshc_cq_full(ufshc_t *u, u8 qid)
{
    ufshc_queue_t *q = &u->queues[qid];
    if (!q->cq_en || q->cq_depth == 0) {
        return true;
    }
    u32 bytes = q->cq_depth * UFSHC_CQ_ENTRY_SIZE;
    u32 next = (u->mcq_op[qid].cq_tp + UFSHC_CQ_ENTRY_SIZE) % bytes;
    return next == u->mcq_op[qid].cq_hp;
}

static void ufshc_update_queue_cfg(ufshc_t *u, u32 qid)
{
    ufshc_mcq_cfg_t *cfg = &u->mcq_cfg[qid];
    ufshc_queue_t *q = &u->queues[qid];
    u32 sq_bytes = ((cfg->sqattr & UFSHC_SQATTR_SIZE_MASK) + 1u) << 2u;
    u32 cq_bytes = ((cfg->cqattr & UFSHC_CQATTR_SIZE_MASK) + 1u) << 2u;

    q->cqid = (u8)((cfg->sqattr & UFSHC_SQATTR_CQID_MASK) >>
                   UFSHC_SQATTR_CQID_SHIFT);
    q->sq_base = cfg->sqlba;
    q->cq_base = cfg->cqlba;
    q->sq_depth = sq_bytes / UFSHC_SQ_ENTRY_SIZE;
    q->cq_depth = cq_bytes / UFSHC_CQ_ENTRY_SIZE;
    if (q->sq_depth == 0) {
        q->sq_depth = 1;
    }
    if (q->cq_depth == 0) {
        q->cq_depth = 1;
    }
    q->cq_en = (cfg->cqattr & UFSHC_CQATTR_CQEN) != 0;
    q->sq_en = (cfg->sqattr & UFSHC_SQATTR_SQEN) != 0 &&
               q->cqid < UFSHC_MAX_QNUM && u->queues[q->cqid].cq_en;
}

static void ufshc_build_upiu_header(ufshc_t *u, u8 trans_type, u8 flags,
                                    u8 response, u8 scsi_status,
                                    u16 data_segment_length)
{
    (void)u;
    ufshc_active_req_t *r = &u->req;
    memcpy(&r->rsp_upiu.header, &r->req_upiu.header,
           sizeof(r->rsp_upiu.header));
    r->rsp_upiu.header.trans_type = trans_type;
    r->rsp_upiu.header.flags = flags;
    r->rsp_upiu.header.response = response;
    r->rsp_upiu.header.scsi_status = scsi_status;
    r->rsp_upiu.header.device_inf = 0;
    r->rsp_upiu.header.data_segment_length =
        cpu_to_be16(data_segment_length);
}

static void ufshc_prepare_rsp_write_len(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    u32 data_len = be16_to_cpu(r->rsp_upiu.header.data_segment_length);
    u32 copy_len = sizeof(ufshc_upiu_header_t) + 20u + data_len;
    if (r->rsp_upiu.header.trans_type == UFSHC_UPIU_NOP_IN) {
        copy_len = sizeof(ufshc_upiu_header_t) + 20u;
    }
    if (copy_len > sizeof(r->rsp_upiu)) {
        copy_len = sizeof(r->rsp_upiu);
    }
    if (r->rsp_upiu_len != 0 && copy_len > r->rsp_upiu_len) {
        copy_len = r->rsp_upiu_len;
    }
    r->rsp_write_len = copy_len;
}

static u32 ufshc_prdt_total_len(ufshc_t *u)
{
    u32 total = 0;
    for (u32 i = 0; i < u->req.prdt_len; i++) {
        total += (le32_to_cpu(u->req.prdt[i].size) & 0x3ffffu) + 1u;
    }
    return total;
}

static void ufshc_build_scsi_rsp(ufshc_t *u, u32 transferred, u8 status)
{
    ufshc_active_req_t *r = &u->req;
    u32 expected = be32_to_cpu(r->req_upiu.sc.exp_data_transfer_len);
    u8 flags = 0;
    u8 response = UFSHC_COMMAND_RESULT_SUCCESS;
    u8 sense[UFSHC_SENSE_SIZE] = {0};

    memset(&r->rsp_upiu, 0, sizeof(r->rsp_upiu));
    if (expected > transferred) {
        r->rsp_upiu.sr.residual_transfer_count =
            cpu_to_be32(expected - transferred);
        flags |= UFSHC_UPIU_FLAG_UNDERFLOW;
    } else if (expected < transferred) {
        r->rsp_upiu.sr.residual_transfer_count =
            cpu_to_be32(transferred - expected);
        flags |= UFSHC_UPIU_FLAG_OVERFLOW;
    }

    if (status != UFSHC_SCSI_GOOD) {
        response = UFSHC_COMMAND_RESULT_FAIL;
        sense[0] = 0x70;
        sense[2] = 0x05;
        sense[7] = 0x0a;
        sense[12] = 0x20;
        sense[13] = 0x00;
        r->rsp_upiu.sr.sense_data_len = cpu_to_be16(sizeof(sense));
        memcpy(r->rsp_upiu.sr.sense_data, sense, sizeof(sense));
    }

    ufshc_build_upiu_header(u, UFSHC_UPIU_RESPONSE, flags, response, status,
                            UFSHC_SENSE_SIZE + 2u);
    ufshc_prepare_rsp_write_len(u);
}

static void ufshc_set_query_fields(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    r->rsp_upiu.qr.opcode = r->req_upiu.qr.opcode;
    r->rsp_upiu.qr.idn = r->req_upiu.qr.idn;
    r->rsp_upiu.qr.index = r->req_upiu.qr.index;
    r->rsp_upiu.qr.selector = r->req_upiu.qr.selector;
}

static void ufshc_copy_query_desc(ufshc_t *u, const u8 *src, u32 src_len)
{
    ufshc_active_req_t *r = &u->req;
    u32 req_len = be16_to_cpu(r->req_upiu.qr.length);
    u32 len = MIN(req_len, src_len);
    memcpy(r->rsp_upiu.qr.data, src, len);
    r->rsp_upiu.qr.length = cpu_to_be16((u16)len);
}

static u32 ufshc_query_read_desc(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    u8 idn = r->req_upiu.qr.idn;
    u8 index = r->req_upiu.qr.index;
    u8 desc[128];

    if (r->req_upiu.qr.selector != 0) {
        return UFSHC_QUERY_RESULT_INVALID_SELECTOR;
    }

    switch (idn) {
    case UFSHC_DESC_IDN_DEVICE:
        ufshc_copy_query_desc(u, u->device_desc, sizeof(u->device_desc));
        return UFSHC_QUERY_RESULT_SUCCESS;
    case UFSHC_DESC_IDN_UNIT:
        if (index == 0) {
            ufshc_copy_query_desc(u, u->unit_desc, sizeof(u->unit_desc));
            return UFSHC_QUERY_RESULT_SUCCESS;
        }
        if (index == UFSHC_WLUN_RPMB) {
            memset(desc, 0, sizeof(desc));
            desc[0] = 0x23;
            desc[1] = UFSHC_DESC_IDN_UNIT;
            desc[2] = UFSHC_WLUN_RPMB;
            ufshc_copy_query_desc(u, desc, desc[0]);
            return UFSHC_QUERY_RESULT_SUCCESS;
        }
        return UFSHC_QUERY_RESULT_INVALID_INDEX;
    case UFSHC_DESC_IDN_GEOMETRY:
        ufshc_copy_query_desc(u, u->geometry_desc, sizeof(u->geometry_desc));
        return UFSHC_QUERY_RESULT_SUCCESS;
    case UFSHC_DESC_IDN_INTERCONNECT:
        memset(desc, 0, sizeof(desc));
        desc[0] = 0x06;
        desc[1] = UFSHC_DESC_IDN_INTERCONNECT;
        store_be16(&desc[2], 0x0180);
        store_be16(&desc[4], 0x0410);
        ufshc_copy_query_desc(u, desc, desc[0]);
        return UFSHC_QUERY_RESULT_SUCCESS;
    case UFSHC_DESC_IDN_STRING:
        memset(desc, 0, sizeof(desc));
        desc[1] = UFSHC_DESC_IDN_STRING;
        if (index == 0) {
            const char *s = "RVBUCKET";
            desc[0] = 2 + 2 * 8;
            for (u32 i = 0; i < 8; i++) {
                store_be16(&desc[2 + i * 2], (u16)s[i]);
            }
        } else if (index == 1) {
            const char *s = "RVBUCKET UFS";
            desc[0] = 2 + 2 * 12;
            for (u32 i = 0; i < 12; i++) {
                store_be16(&desc[2 + i * 2], (u16)s[i]);
            }
        } else if (index == 4) {
            const char *s = "0001";
            desc[0] = 2 + 2 * 4;
            for (u32 i = 0; i < 4; i++) {
                store_be16(&desc[2 + i * 2], (u16)s[i]);
            }
        } else {
            desc[0] = 2;
        }
        ufshc_copy_query_desc(u, desc, desc[0]);
        return UFSHC_QUERY_RESULT_SUCCESS;
    case UFSHC_DESC_IDN_POWER:
        memset(desc, 0, sizeof(desc));
        desc[0] = 0x62;
        desc[1] = UFSHC_DESC_IDN_POWER;
        ufshc_copy_query_desc(u, desc, desc[0]);
        return UFSHC_QUERY_RESULT_SUCCESS;
    case UFSHC_DESC_IDN_HEALTH:
        memset(desc, 0, sizeof(desc));
        desc[0] = 0x25;
        desc[1] = UFSHC_DESC_IDN_HEALTH;
        ufshc_copy_query_desc(u, desc, desc[0]);
        return UFSHC_QUERY_RESULT_SUCCESS;
    default:
        r->rsp_upiu.qr.length = 0;
        return UFSHC_QUERY_RESULT_INVALID_IDN;
    }
}

static u32 ufshc_query_attr(ufshc_t *u, bool write)
{
    ufshc_active_req_t *r = &u->req;
    u8 idn = r->req_upiu.qr.idn;
    if (idn >= UFSHC_ATTR_IDN_COUNT) {
        return UFSHC_QUERY_RESULT_INVALID_IDN;
    }

    if (write) {
        switch (idn) {
        case UFSHC_ATTR_IDN_ACTIVE_ICC_LVL:
        case UFSHC_ATTR_IDN_MAX_DATA_IN:
        case UFSHC_ATTR_IDN_MAX_DATA_OUT:
        case UFSHC_ATTR_IDN_REF_CLK_FREQ:
        case UFSHC_ATTR_IDN_MAX_NUM_RTT:
        case UFSHC_ATTR_IDN_EE_CONTROL:
        case UFSHC_ATTR_IDN_SECONDS_PASSED:
        case UFSHC_ATTR_IDN_PSA_STATE:
        case UFSHC_ATTR_IDN_PSA_DATA_SIZE:
        case UFSHC_ATTR_IDN_TIMESTAMP:
            u->attrs[idn] = be32_to_cpu(r->req_upiu.qr.value);
            break;
        default:
            return UFSHC_QUERY_RESULT_NOT_WRITEABLE;
        }
    }
    r->rsp_upiu.qr.value = cpu_to_be32(u->attrs[idn]);
    return UFSHC_QUERY_RESULT_SUCCESS;
}

static u32 ufshc_query_flag(ufshc_t *u, u32 op)
{
    ufshc_active_req_t *r = &u->req;
    u8 idn = r->req_upiu.qr.idn;
    u8 value;

    if (idn >= UFSHC_FLAG_IDN_COUNT) {
        return UFSHC_QUERY_RESULT_INVALID_IDN;
    }

    switch (op) {
    case UFSHC_QUERY_OPCODE_READ_FLAG:
        break;
    case UFSHC_QUERY_OPCODE_SET_FLAG:
        if (idn == UFSHC_FLAG_IDN_FDEVICEINIT ||
            idn == UFSHC_FLAG_IDN_BKOPS_EN ||
            idn == UFSHC_FLAG_IDN_LIFE_SPAN_EN) {
            u->flags[idn] = 1;
        } else {
            return UFSHC_QUERY_RESULT_NOT_WRITEABLE;
        }
        break;
    case UFSHC_QUERY_OPCODE_CLEAR_FLAG:
        if (idn == UFSHC_FLAG_IDN_BKOPS_EN ||
            idn == UFSHC_FLAG_IDN_LIFE_SPAN_EN) {
            u->flags[idn] = 0;
        } else {
            return UFSHC_QUERY_RESULT_NOT_WRITEABLE;
        }
        break;
    case UFSHC_QUERY_OPCODE_TOGGLE_FLAG:
        if (idn == UFSHC_FLAG_IDN_BKOPS_EN ||
            idn == UFSHC_FLAG_IDN_LIFE_SPAN_EN) {
            u->flags[idn] = !u->flags[idn];
        } else {
            return UFSHC_QUERY_RESULT_NOT_WRITEABLE;
        }
        break;
    default:
        return UFSHC_QUERY_RESULT_INVALID_OPCODE;
    }

    value = (idn == UFSHC_FLAG_IDN_FDEVICEINIT) ? 0 : u->flags[idn];
    r->rsp_upiu.qr.value = cpu_to_be32(value);
    return UFSHC_QUERY_RESULT_SUCCESS;
}

static void ufshc_exec_query(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    u32 status = UFSHC_QUERY_RESULT_GENERAL_FAILURE;

    memset(&r->rsp_upiu, 0, sizeof(r->rsp_upiu));
    if (r->req_upiu.header.query_func == UFSHC_UPIU_QUERY_READ) {
        switch (r->req_upiu.qr.opcode) {
        case UFSHC_QUERY_OPCODE_NOP:
            status = UFSHC_QUERY_RESULT_SUCCESS;
            break;
        case UFSHC_QUERY_OPCODE_READ_DESC:
            status = ufshc_query_read_desc(u);
            break;
        case UFSHC_QUERY_OPCODE_READ_ATTR:
            status = ufshc_query_attr(u, false);
            break;
        case UFSHC_QUERY_OPCODE_READ_FLAG:
            status = ufshc_query_flag(u, UFSHC_QUERY_OPCODE_READ_FLAG);
            break;
        default:
            status = UFSHC_QUERY_RESULT_INVALID_OPCODE;
            break;
        }
    } else if (r->req_upiu.header.query_func == UFSHC_UPIU_QUERY_WRITE) {
        switch (r->req_upiu.qr.opcode) {
        case UFSHC_QUERY_OPCODE_NOP:
            status = UFSHC_QUERY_RESULT_SUCCESS;
            break;
        case UFSHC_QUERY_OPCODE_WRITE_ATTR:
            status = ufshc_query_attr(u, true);
            break;
        case UFSHC_QUERY_OPCODE_SET_FLAG:
        case UFSHC_QUERY_OPCODE_CLEAR_FLAG:
        case UFSHC_QUERY_OPCODE_TOGGLE_FLAG:
            status = ufshc_query_flag(u, r->req_upiu.qr.opcode);
            break;
        default:
            status = UFSHC_QUERY_RESULT_INVALID_OPCODE;
            break;
        }
    }

    ufshc_set_query_fields(u);
    ufshc_build_upiu_header(u, UFSHC_UPIU_QUERY_RSP, 0, (u8)status, 0,
                            be16_to_cpu(r->rsp_upiu.qr.length));
    ufshc_prepare_rsp_write_len(u);
    r->req_result = (status == UFSHC_QUERY_RESULT_SUCCESS) ?
        UFSHC_OCS_SUCCESS : UFSHC_OCS_INVALID_CMD_TABLE;
}

static void ufshc_prepare_data_to_host(ufshc_t *u, const u8 *src, u32 len)
{
    ufshc_active_req_t *r = &u->req;
    u32 expected = be32_to_cpu(r->req_upiu.sc.exp_data_transfer_len);
    u32 prdt_len = ufshc_prdt_total_len(u);
    u32 copy_len = MIN(len, expected);
    copy_len = MIN(copy_len, prdt_len);
    copy_len = MIN(copy_len, u->xfer_buf_size);
    memset(u->xfer_buf, 0, copy_len);
    memcpy(u->xfer_buf, src, copy_len);
    r->data_dir = UFSHC_DATA_TO_HOST;
    r->data_len = copy_len;
    r->data_pos = 0;
    r->prdt_idx = 0;
    r->prdt_off = 0;
    ufshc_build_scsi_rsp(u, copy_len, UFSHC_SCSI_GOOD);
}

static void ufshc_exec_wlun_scsi(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    u8 out[256];
    u32 len = 0;
    u8 status = UFSHC_SCSI_GOOD;

    memset(out, 0, sizeof(out));
    switch (r->req_upiu.sc.cdb[0]) {
    case UFSHC_SCSI_REPORT_LUNS:
        len = 16;
        store_be32(out, 8);
        out[9] = 0;
        break;
    case UFSHC_SCSI_INQUIRY:
        if (r->req_upiu.sc.cdb[1] & 1u) {
            u8 page = r->req_upiu.sc.cdb[2];
            out[0] = 0x1e;
            out[1] = page;
            if (page == 0x00) {
                len = 7;
                out[3] = 3;
                out[4] = 0x00;
                out[5] = 0x80;
                out[6] = 0x83;
            } else if (page == 0x80) {
                len = 8;
                out[3] = 4;
                memcpy(&out[4], "0001", 4);
            } else if (page == 0x83) {
                static const char id[] = "RVBUCKET-UFS-0001";
                len = 8 + sizeof(id) - 1u;
                store_be16(&out[2], (u16)(len - 4u));
                out[4] = 0x02;
                out[5] = 0x01;
                out[7] = sizeof(id) - 1u;
                memcpy(&out[8], id, sizeof(id) - 1u);
            } else {
                status = UFSHC_SCSI_CHECK_CONDITION;
            }
        } else {
            len = 36;
            out[0] = 0x1e;
            out[2] = 0x06;
            out[3] = 0x02;
            out[4] = 31;
            memcpy(&out[8], "RVBUCKET", 8);
            memcpy(&out[16], "RVBUCKET UFS     ", 16);
            memcpy(&out[32], "0001", 4);
        }
        break;
    case UFSHC_SCSI_REQUEST_SENSE:
        len = UFSHC_SENSE_SIZE;
        out[0] = 0x70;
        out[7] = 0x0a;
        break;
    case UFSHC_SCSI_START_STOP:
    case UFSHC_SCSI_TEST_UNIT_READY:
        len = 0;
        break;
    default:
        status = UFSHC_SCSI_CHECK_CONDITION;
        break;
    }

    if (status == UFSHC_SCSI_GOOD && len > 0) {
        ufshc_prepare_data_to_host(u, out, len);
    } else {
        r->data_dir = UFSHC_DATA_NONE;
        r->data_len = 0;
        ufshc_build_scsi_rsp(u, 0, status);
    }
    r->req_result = (status == UFSHC_SCSI_GOOD) ?
        UFSHC_OCS_SUCCESS : UFSHC_OCS_INVALID_CMD_TABLE;
}

static bool ufshc_scsi_rw_10(ufshc_t *u, bool write)
{
    ufshc_active_req_t *r = &u->req;
    u32 lba = load_be32(&r->req_upiu.sc.cdb[2]);
    u32 blocks = ((u32)r->req_upiu.sc.cdb[7] << 8) | r->req_upiu.sc.cdb[8];
    u32 bytes = blocks << UFSHC_BLOCK_SHIFT;
    u32 off = lba << UFSHC_BLOCK_SHIFT;
    if (off > u->storage_size || bytes > u->storage_size - off ||
        bytes > u->xfer_buf_size) {
        return false;
    }
    r->data_lba = lba;
    r->data_len = MIN(bytes, ufshc_prdt_total_len(u));
    r->data_len = MIN(r->data_len, be32_to_cpu(r->req_upiu.sc.exp_data_transfer_len));
    r->data_dir = write ? UFSHC_DATA_FROM_HOST : UFSHC_DATA_TO_HOST;
    r->data_pos = 0;
    r->prdt_idx = 0;
    r->prdt_off = 0;
    if (!write && !ufshc_backend_read(u, off, u->xfer_buf, r->data_len)) {
        return false;
    }
    return true;
}

static bool ufshc_scsi_rw_16(ufshc_t *u, bool write)
{
    ufshc_active_req_t *r = &u->req;
    u64 lba64 = load_be64(&r->req_upiu.sc.cdb[2]);
    u32 blocks = load_be32(&r->req_upiu.sc.cdb[10]);
    u64 off64 = lba64 << UFSHC_BLOCK_SHIFT;
    u32 bytes = blocks << UFSHC_BLOCK_SHIFT;
    if (lba64 > 0xffffffffu || off64 > u->storage_size ||
        bytes > u->storage_size - (u32)off64 || bytes > u->xfer_buf_size) {
        return false;
    }
    r->data_lba = (u32)lba64;
    r->data_len = MIN(bytes, ufshc_prdt_total_len(u));
    r->data_len = MIN(r->data_len, be32_to_cpu(r->req_upiu.sc.exp_data_transfer_len));
    r->data_dir = write ? UFSHC_DATA_FROM_HOST : UFSHC_DATA_TO_HOST;
    r->data_pos = 0;
    r->prdt_idx = 0;
    r->prdt_off = 0;
    if (!write &&
        !ufshc_backend_read(u, (u32)off64, u->xfer_buf, r->data_len)) {
        return false;
    }
    return true;
}

static void ufshc_exec_lu_scsi(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    u8 out[256];
    u32 len = 0;
    u8 status = UFSHC_SCSI_GOOD;
    bool data_prepared = false;

    memset(out, 0, sizeof(out));
    switch (r->req_upiu.sc.cdb[0]) {
    case UFSHC_SCSI_TEST_UNIT_READY:
    case UFSHC_SCSI_START_STOP:
    case UFSHC_SCSI_SYNCHRONIZE_CACHE:
    case UFSHC_SCSI_UNMAP:
        len = 0;
        break;
    case UFSHC_SCSI_REQUEST_SENSE:
        len = UFSHC_SENSE_SIZE;
        out[0] = 0x70;
        out[7] = 0x0a;
        break;
    case UFSHC_SCSI_INQUIRY:
        if (r->req_upiu.sc.cdb[1] & 1u) {
            u8 page = r->req_upiu.sc.cdb[2];
            out[0] = 0x00;
            out[1] = page;
            if (page == 0x00) {
                len = 7;
                out[3] = 3;
                out[4] = 0x00;
                out[5] = 0x80;
                out[6] = 0x83;
            } else if (page == 0x80) {
                len = 8;
                out[3] = 4;
                memcpy(&out[4], "0001", 4);
            } else if (page == 0x83) {
                static const char id[] = "RVBUCKET-UFS-LU0";
                len = 8 + sizeof(id) - 1u;
                store_be16(&out[2], (u16)(len - 4u));
                out[4] = 0x02;
                out[5] = 0x01;
                out[7] = sizeof(id) - 1u;
                memcpy(&out[8], id, sizeof(id) - 1u);
            } else {
                status = UFSHC_SCSI_CHECK_CONDITION;
            }
        } else {
            len = 36;
            out[0] = 0x00;
            out[2] = 0x06;
            out[3] = 0x02;
            out[4] = 31;
            memcpy(&out[8], "RVBUCKET", 8);
            memcpy(&out[16], "RVBUCKET UFS     ", 16);
            memcpy(&out[32], "0001", 4);
        }
        break;
    case UFSHC_SCSI_MODE_SENSE_6:
        len = 4;
        out[0] = 3;
        break;
    case UFSHC_SCSI_MODE_SENSE_10:
        len = 8;
        out[1] = 6;
        break;
    case UFSHC_SCSI_READ_CAPACITY_10: {
        if (u->storage_size < UFSHC_BLOCK_SIZE) {
            status = UFSHC_SCSI_CHECK_CONDITION;
            break;
        }
        u32 blocks = u->storage_size >> UFSHC_BLOCK_SHIFT;
        len = 8;
        store_be32(out, blocks - 1u);
        store_be32(out + 4, UFSHC_BLOCK_SIZE);
        break;
    }
    case UFSHC_SCSI_SERVICE_ACTION_IN_16:
        if (r->req_upiu.sc.cdb[1] == UFSHC_SCSI_READ_CAPACITY_16_SA) {
            if (u->storage_size < UFSHC_BLOCK_SIZE) {
                status = UFSHC_SCSI_CHECK_CONDITION;
                break;
            }
            u64 blocks = u->storage_size >> UFSHC_BLOCK_SHIFT;
            len = 32;
            store_be64(out, blocks - 1u);
            store_be32(out + 8, UFSHC_BLOCK_SIZE);
        } else {
            status = UFSHC_SCSI_CHECK_CONDITION;
        }
        break;
    case UFSHC_SCSI_REPORT_LUNS:
        len = 16;
        store_be32(out, 8);
        out[9] = 0;
        break;
    case UFSHC_SCSI_READ_10:
        if (!ufshc_scsi_rw_10(u, false)) {
            status = UFSHC_SCSI_CHECK_CONDITION;
        } else {
            data_prepared = true;
        }
        break;
    case UFSHC_SCSI_WRITE_10:
        if (!ufshc_scsi_rw_10(u, true)) {
            status = UFSHC_SCSI_CHECK_CONDITION;
        } else {
            data_prepared = true;
        }
        break;
    case UFSHC_SCSI_READ_16:
        if (!ufshc_scsi_rw_16(u, false)) {
            status = UFSHC_SCSI_CHECK_CONDITION;
        } else {
            data_prepared = true;
        }
        break;
    case UFSHC_SCSI_WRITE_16:
        if (!ufshc_scsi_rw_16(u, true)) {
            status = UFSHC_SCSI_CHECK_CONDITION;
        } else {
            data_prepared = true;
        }
        break;
    default:
        status = UFSHC_SCSI_CHECK_CONDITION;
        break;
    }

    if (data_prepared) {
        ufshc_build_scsi_rsp(u, r->data_len, UFSHC_SCSI_GOOD);
    } else if (status == UFSHC_SCSI_GOOD && len > 0) {
        ufshc_prepare_data_to_host(u, out, len);
    } else {
        r->data_dir = UFSHC_DATA_NONE;
        r->data_len = 0;
        ufshc_build_scsi_rsp(u, 0, status);
    }
    r->req_result = (status == UFSHC_SCSI_GOOD) ?
        UFSHC_OCS_SUCCESS : UFSHC_OCS_INVALID_CMD_TABLE;
}

static void ufshc_exec_scsi(ufshc_t *u)
{
    u8 lun = u->req.req_upiu.header.lun;
    if (lun == UFSHC_WLUN_REPORT_LUNS || lun == UFSHC_WLUN_UFS_DEVICE ||
        lun == UFSHC_WLUN_BOOT || lun == UFSHC_WLUN_RPMB) {
        ufshc_exec_wlun_scsi(u);
    } else if (lun == 0) {
        ufshc_exec_lu_scsi(u);
    } else {
        u->req.data_dir = UFSHC_DATA_NONE;
        u->req.data_len = 0;
        ufshc_build_scsi_rsp(u, 0, UFSHC_SCSI_CHECK_CONDITION);
        u->req.req_result = UFSHC_OCS_INVALID_CMD_TABLE;
    }
}

static bool ufshc_start_next_prdt_dma(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;
    while (r->prdt_idx < r->prdt_len && r->data_pos < r->data_len) {
        ufshc_prdt_t *p = &r->prdt[r->prdt_idx];
        u32 seg_len = (le32_to_cpu(p->size) & 0x3ffffu) + 1u;
        u32 seg_addr = (u32)le64_to_cpu(p->addr);
        if (r->prdt_off >= seg_len) {
            r->prdt_idx++;
            r->prdt_off = 0;
            continue;
        }
        u32 chunk = MIN(seg_len - r->prdt_off, r->data_len - r->data_pos);
        if (r->data_dir == UFSHC_DATA_TO_HOST) {
            ufshc_dma_start(u, true, seg_addr + r->prdt_off,
                            u->xfer_buf + r->data_pos, chunk);
        } else {
            ufshc_dma_start(u, false, seg_addr + r->prdt_off,
                            u->xfer_buf + r->data_pos, chunk);
        }
        return true;
    }
    return false;
}

static void ufshc_req_start_rsp(ufshc_t *u)
{
    u->req.utrd.dword_2 = cpu_to_le32(u->req.req_result & UFSHC_OCS_MASK);
    ufshc_dma_start(u, true, u->req.rsp_upiu_addr, (u8 *)&u->req.rsp_upiu,
                    u->req.rsp_write_len);
    u->req_state = UFSHC_REQ_WRITE_RSP;
}

static void ufshc_req_clock(ufshc_t *u)
{
    ufshc_active_req_t *r = &u->req;

    switch (u->req_state) {
    case UFSHC_REQ_IDLE:
        for (u32 qid = 0; qid < UFSHC_MAX_QNUM; qid++) {
            ufshc_queue_t *q = &u->queues[qid];
            if (!q->sq_en || !q->cq_en || q->sq_depth == 0 ||
                q->cq_depth == 0 || ufshc_cq_full(u, qid)) {
                continue;
            }
            if (u->mcq_op[qid].sq_hp == u->mcq_op[qid].sq_tp) {
                continue;
            }
            UFSHC_LOG(u, "q%u start SQE sq_base=%08x hp=%08x tp=%08x cq_hp=%08x cq_tp=%08x\n",
                      qid, q->sq_base, u->mcq_op[qid].sq_hp,
                      u->mcq_op[qid].sq_tp, u->mcq_op[qid].cq_hp,
                      u->mcq_op[qid].cq_tp);
            memset(r, 0, sizeof(*r));
            r->qid = (u8)qid;
            ufshc_dma_start(u, false, q->sq_base + u->mcq_op[qid].sq_hp,
                            (u8 *)&r->utrd, sizeof(r->utrd));
            u->req_state = UFSHC_REQ_READ_SQE;
            return;
        }
        break;
    case UFSHC_REQ_READ_SQE:
        if (!ufshc_dma_done(u)) {
            break;
        }
        if (!u->dma.ok) {
            u->req_state = UFSHC_REQ_IDLE;
            break;
        }
        {
            ufshc_queue_t *q = &u->queues[r->qid];
            u32 sq_bytes = q->sq_depth * UFSHC_SQ_ENTRY_SIZE;
            u->mcq_op[r->qid].sq_hp =
                (u->mcq_op[r->qid].sq_hp + UFSHC_SQ_ENTRY_SIZE) % sq_bytes;
        }
        r->cmd_desc_base = le32_to_cpu(r->utrd.command_desc_base_addr_lo);
        r->rsp_upiu_addr = r->cmd_desc_base +
            le16_to_cpu(r->utrd.response_upiu_offset) * sizeof(u32);
        r->rsp_upiu_len =
            le16_to_cpu(r->utrd.response_upiu_length) * sizeof(u32);
        r->prdt_base = r->cmd_desc_base +
            le16_to_cpu(r->utrd.prd_table_offset) * sizeof(u32);
        r->prdt_len = le16_to_cpu(r->utrd.prd_table_length);
        if (r->prdt_len > UFSHC_MAX_PRDT) {
            r->prdt_len = UFSHC_MAX_PRDT;
        }
        UFSHC_LOG(u,
                  "q%u SQE cmd_desc=%08x rsp_addr=%08x rsp_len=%u prdt_base=%08x prdt_len=%u dword0=%08x dword2=%08x\n",
                  r->qid, r->cmd_desc_base, r->rsp_upiu_addr,
                  r->rsp_upiu_len, r->prdt_base, r->prdt_len,
                  le32_to_cpu(r->utrd.dword_0), le32_to_cpu(r->utrd.dword_2));
        ufshc_dma_start(u, false, r->cmd_desc_base, (u8 *)&r->req_upiu,
                        sizeof(r->req_upiu));
        u->req_state = UFSHC_REQ_READ_UPIU;
        break;
    case UFSHC_REQ_READ_UPIU:
        if (!ufshc_dma_done(u)) {
            break;
        }
        if (r->prdt_len != 0) {
            ufshc_dma_start(u, false, r->prdt_base, (u8 *)r->prdt,
                            r->prdt_len * sizeof(r->prdt[0]));
            u->req_state = UFSHC_REQ_READ_PRDT;
        } else {
            u->req_state = UFSHC_REQ_EXEC;
        }
        UFSHC_LOG(u,
                  "q%u UPIU type=%02x flags=%02x lun=%02x tag=%u qfunc=%02x cdb0=%02x qop=%02x idn=%02x len=%u\n",
                  r->qid, r->req_upiu.header.trans_type,
                  r->req_upiu.header.flags, r->req_upiu.header.lun,
                  r->req_upiu.header.task_tag,
                  r->req_upiu.header.query_func, r->req_upiu.sc.cdb[0],
                  r->req_upiu.qr.opcode, r->req_upiu.qr.idn,
                  be16_to_cpu(r->req_upiu.qr.length));
        break;
    case UFSHC_REQ_READ_PRDT:
        if (!ufshc_dma_done(u)) {
            break;
        }
        u->req_state = UFSHC_REQ_EXEC;
        break;
    case UFSHC_REQ_EXEC:
        memset(&r->rsp_upiu, 0, sizeof(r->rsp_upiu));
        r->req_result = UFSHC_OCS_SUCCESS;
        switch (r->req_upiu.header.trans_type) {
        case UFSHC_UPIU_NOP_OUT:
            ufshc_build_upiu_header(u, UFSHC_UPIU_NOP_IN, 0, 0, 0, 0);
            ufshc_prepare_rsp_write_len(u);
            UFSHC_LOG(u, "q%u exec NOP rsp_len=%u\n", r->qid, r->rsp_write_len);
            ufshc_req_start_rsp(u);
            break;
        case UFSHC_UPIU_QUERY_REQ:
            ufshc_exec_query(u);
            UFSHC_LOG(u, "q%u exec QUERY op=%02x idn=%02x rsp=%02x result=%u rsp_len=%u\n",
                      r->qid, r->req_upiu.qr.opcode, r->req_upiu.qr.idn,
                      r->rsp_upiu.header.response, r->req_result,
                      r->rsp_write_len);
            ufshc_req_start_rsp(u);
            break;
        case UFSHC_UPIU_COMMAND:
            ufshc_exec_scsi(u);
            UFSHC_LOG(u, "q%u exec SCSI lun=%02x cdb0=%02x dir=%u data_len=%u result=%u rsp_len=%u\n",
                      r->qid, r->req_upiu.header.lun, r->req_upiu.sc.cdb[0],
                      r->data_dir, r->data_len, r->req_result,
                      r->rsp_write_len);
            if (r->data_len != 0 && r->data_dir != UFSHC_DATA_NONE) {
                if (ufshc_start_next_prdt_dma(u)) {
                    u->req_state = UFSHC_REQ_PRDT_DATA;
                } else {
                    ufshc_req_start_rsp(u);
                }
            } else {
                ufshc_req_start_rsp(u);
            }
            break;
        default:
            r->data_dir = UFSHC_DATA_NONE;
            r->data_len = 0;
            r->req_result = UFSHC_OCS_INVALID_CMD_TABLE;
            ufshc_build_upiu_header(u, UFSHC_UPIU_RESPONSE, 0,
                                    UFSHC_COMMAND_RESULT_FAIL, 0, 0);
            ufshc_prepare_rsp_write_len(u);
            ufshc_req_start_rsp(u);
            break;
        }
        break;
    case UFSHC_REQ_PRDT_DATA:
        if (!ufshc_dma_done(u)) {
            break;
        }
        {
            ufshc_prdt_t *p = &r->prdt[r->prdt_idx];
            u32 seg_len = (le32_to_cpu(p->size) & 0x3ffffu) + 1u;
            u32 chunk = MIN(seg_len - r->prdt_off, r->data_len - r->data_pos);
            r->data_pos += chunk;
            r->prdt_off += chunk;
            if (r->prdt_off >= seg_len) {
                r->prdt_idx++;
                r->prdt_off = 0;
            }
        }
        if (r->data_pos >= r->data_len) {
            if (r->data_dir == UFSHC_DATA_FROM_HOST) {
                u32 off = r->data_lba << UFSHC_BLOCK_SHIFT;
                if (!ufshc_backend_write(u, off, u->xfer_buf, r->data_len)) {
                    r->req_result = UFSHC_OCS_INVALID_PRDT;
                    ufshc_build_scsi_rsp(u, 0, UFSHC_SCSI_CHECK_CONDITION);
                }
            }
            ufshc_req_start_rsp(u);
        } else if (!ufshc_start_next_prdt_dma(u)) {
            ufshc_req_start_rsp(u);
        }
        break;
    case UFSHC_REQ_WRITE_RSP:
        if (!ufshc_dma_done(u)) {
            break;
        }
        {
            ufshc_queue_t *q = &u->queues[r->qid];
            u64 utp_addr = ((u64)r->cmd_desc_base) | r->qid;
            memset(&r->cqe, 0, sizeof(r->cqe));
            r->cqe.utp_addr = cpu_to_le64(utp_addr);
            r->cqe.resp_len = r->utrd.response_upiu_length;
            r->cqe.resp_off = r->utrd.response_upiu_offset;
            r->cqe.prdt_len = r->utrd.prd_table_length;
            r->cqe.prdt_off = r->utrd.prd_table_offset;
            r->cqe.status = (u8)(le32_to_cpu(r->utrd.dword_2) &
                                 UFSHC_OCS_MASK);
            r->cqe.error = 0;
            UFSHC_LOG(u, "q%u write CQE addr=%08x utp=%08llx status=%u cq_tp=%08x\n",
                      r->qid, q->cq_base + u->mcq_op[r->qid].cq_tp,
                      (unsigned long long)le64_to_cpu(r->cqe.utp_addr),
                      r->cqe.status, u->mcq_op[r->qid].cq_tp);
            ufshc_dma_start(u, true, q->cq_base + u->mcq_op[r->qid].cq_tp,
                            (u8 *)&r->cqe, sizeof(r->cqe));
            u->req_state = UFSHC_REQ_WRITE_CQE;
        }
        break;
    case UFSHC_REQ_WRITE_CQE:
        if (!ufshc_dma_done(u)) {
            break;
        }
        {
            ufshc_queue_t *q = &u->queues[r->qid];
            u32 cq_bytes = q->cq_depth * UFSHC_CQ_ENTRY_SIZE;
            u->mcq_op[r->qid].cq_tp =
                (u->mcq_op[r->qid].cq_tp + UFSHC_CQ_ENTRY_SIZE) % cq_bytes;
            u->mcq_op[r->qid].cq_is |= UFSHC_CQIS_TEPS;
            u->is |= UFSHC_IS_CQES;
            UFSHC_LOG(u, "q%u complete CQE new_cq_tp=%08x cq_is=%08x is=%08x ie=%08x\n",
                      r->qid, u->mcq_op[r->qid].cq_tp,
                      u->mcq_op[r->qid].cq_is, u->is, u->ie);
            ufshc_irq_check(u);
            u->req_state = UFSHC_REQ_IDLE;
        }
        break;
    }
}

static void ufshc_process_uiccmd(ufshc_t *u, u32 val)
{
    switch (val) {
    case UFSHC_UIC_CMD_DME_GET:
    case UFSHC_UIC_CMD_DME_PEER_GET:
        switch ((u->ucmdarg1 >> 16) & 0xffffu) {
        case UFSHC_PA_CONNECTEDTXDATALANES:
        case UFSHC_PA_CONNECTEDRXDATALANES:
        case UFSHC_PA_MAXRXPWMGEAR:
        case UFSHC_PA_MAXRXHSGEAR:
            u->ucmdarg3 = 1;
            break;
        default:
            u->ucmdarg3 = 0;
            break;
        }
        u->ucmdarg2 = UFSHC_UIC_RESULT_SUCCESS;
        break;
    case UFSHC_UIC_CMD_DME_SET:
    case UFSHC_UIC_CMD_DME_PEER_SET:
        if (((u->ucmdarg1 >> 16) & 0xffffu) == UFSHC_PA_PWRMODE) {
            u->hcs = (u->hcs & ~(0x7u << UFSHC_HCS_UPMCRS_SHIFT)) |
                     (UFSHC_PWR_LOCAL << UFSHC_HCS_UPMCRS_SHIFT);
            u->is |= UFSHC_IS_UPMS;
        }
        u->ucmdarg2 = UFSHC_UIC_RESULT_SUCCESS;
        break;
    case UFSHC_UIC_CMD_DME_LINK_STARTUP:
        u->hcs |= UFSHC_HCS_DP | UFSHC_HCS_UTRLRDY | UFSHC_HCS_UTMRLRDY;
        u->ucmdarg2 = UFSHC_UIC_RESULT_SUCCESS;
        break;
    case UFSHC_UIC_CMD_DME_HIBER_ENTER:
        u->hcs = (u->hcs & ~(0x7u << UFSHC_HCS_UPMCRS_SHIFT)) |
                 (UFSHC_PWR_LOCAL << UFSHC_HCS_UPMCRS_SHIFT);
        u->is |= UFSHC_IS_UHES;
        u->ucmdarg2 = UFSHC_UIC_RESULT_SUCCESS;
        break;
    case UFSHC_UIC_CMD_DME_HIBER_EXIT:
        u->hcs = (u->hcs & ~(0x7u << UFSHC_HCS_UPMCRS_SHIFT)) |
                 (UFSHC_PWR_LOCAL << UFSHC_HCS_UPMCRS_SHIFT);
        u->is |= UFSHC_IS_UHXS;
        u->ucmdarg2 = UFSHC_UIC_RESULT_SUCCESS;
        break;
    default:
        u->ucmdarg2 = UFSHC_UIC_RESULT_FAILURE;
        break;
    }
    u->uiccmd = val;
    u->is |= UFSHC_IS_UCCS;
    ufshc_irq_check(u);
}

static u32 ufshc_read_main_reg(ufshc_t *u, u32 off)
{
    switch (off) {
    case UFSHC_A_CAP: return u->cap;
    case UFSHC_A_MCQCAP: return u->mcqcap;
    case UFSHC_A_VER: return u->ver;
    case UFSHC_A_HCPID: return u->hcpid;
    case UFSHC_A_HCMID: return u->hcmid;
    case UFSHC_A_AHIT: return u->ahit;
    case UFSHC_A_IS: return u->is;
    case UFSHC_A_IE: return u->ie;
    case UFSHC_A_HCS: return u->hcs;
    case UFSHC_A_HCE: return u->hce;
    case UFSHC_A_UECPA: return u->uecpa;
    case UFSHC_A_UECDL: return u->uecdl;
    case UFSHC_A_UECN: return u->uecn;
    case UFSHC_A_UECT: return u->uect;
    case UFSHC_A_UECDME: return u->uecdme;
    case UFSHC_A_UTRIACR: return u->utriacr;
    case UFSHC_A_UTRLBA: return u->utrlba;
    case UFSHC_A_UTRLBAU: return u->utrlbau;
    case UFSHC_A_UTRLDBR: return u->utrldbr;
    case UFSHC_A_UTRLCLR: return u->utrlclr;
    case UFSHC_A_UTRLRSR: return u->utrlrsr;
    case UFSHC_A_UTRLCNR: return u->utrlcnr;
    case UFSHC_A_UTMRLBA: return u->utmrlba;
    case UFSHC_A_UTMRLBAU: return u->utmrlbau;
    case UFSHC_A_UTMRLDBR: return u->utmrldbr;
    case UFSHC_A_UTMRLCLR: return u->utmrlclr;
    case UFSHC_A_UTMRLRSR: return u->utmrlrsr;
    case UFSHC_A_UICCMD: return u->uiccmd;
    case UFSHC_A_UCMDARG1: return u->ucmdarg1;
    case UFSHC_A_UCMDARG2: return u->ucmdarg2;
    case UFSHC_A_UCMDARG3: return u->ucmdarg3;
    case UFSHC_A_CCAP: return u->ccap;
    case UFSHC_A_CONFIG: return u->config;
    case UFSHC_A_MCQCONFIG: return u->mcqconfig;
    case UFSHC_A_ESILBA: return u->esilba;
    case UFSHC_A_ESIUBA: return u->esiuba;
    default: return 0;
    }
}

static void ufshc_write_main_reg(ufshc_t *u, u32 off, u32 data)
{
    switch (off) {
    case UFSHC_A_IS:
        u->is &= ~data;
        ufshc_irq_check(u);
        break;
    case UFSHC_A_IE:
        u->ie = data;
        UFSHC_LOG(u, "write IE=%08x\n", data);
        ufshc_irq_check(u);
        break;
    case UFSHC_A_HCE:
        UFSHC_LOG(u, "write HCE=%08x\n", data);
        if ((data & UFSHC_HCE_HCE) != 0) {
            u->hce |= UFSHC_HCE_HCE;
            u->hcs |= UFSHC_HCS_UCRDY;
        } else {
            u->hce &= ~UFSHC_HCE_HCE;
            u->hcs = 0;
        }
        break;
    case UFSHC_A_AHIT: u->ahit = data; break;
    case UFSHC_A_UTRIACR: u->utriacr = data; break;
    case UFSHC_A_UTRLBA: u->utrlba = data; break;
    case UFSHC_A_UTRLBAU: u->utrlbau = data; break;
    case UFSHC_A_UTRLDBR: u->utrldbr = data; break;
    case UFSHC_A_UTRLCLR: u->utrlclr = data; break;
    case UFSHC_A_UTRLRSR: u->utrlrsr = data; break;
    case UFSHC_A_UTRLCNR: u->utrlcnr &= ~data; break;
    case UFSHC_A_UTMRLBA: u->utmrlba = data; break;
    case UFSHC_A_UTMRLBAU: u->utmrlbau = data; break;
    case UFSHC_A_UTMRLDBR: u->utmrldbr = data; break;
    case UFSHC_A_UTMRLCLR: u->utmrlclr = data; break;
    case UFSHC_A_UTMRLRSR: u->utmrlrsr = data; break;
    case UFSHC_A_UICCMD:
        UFSHC_LOG(u, "write UICCMD=%08x arg1=%08x arg2=%08x arg3=%08x\n",
                  data, u->ucmdarg1, u->ucmdarg2, u->ucmdarg3);
        ufshc_process_uiccmd(u, data);
        break;
    case UFSHC_A_UCMDARG1: u->ucmdarg1 = data; break;
    case UFSHC_A_UCMDARG2: u->ucmdarg2 = data; break;
    case UFSHC_A_UCMDARG3: u->ucmdarg3 = data; break;
    case UFSHC_A_CONFIG: u->config = data; UFSHC_LOG(u, "write CONFIG=%08x\n", data); break;
    case UFSHC_A_MCQCONFIG: u->mcqconfig = data; UFSHC_LOG(u, "write MCQCONFIG=%08x\n", data); break;
    case UFSHC_A_ESILBA: u->esilba = data; break;
    case UFSHC_A_ESIUBA: u->esiuba = data; break;
    default: break;
    }
}

static bool ufshc_decode_mcq_cfg(u32 off, u32 *qid, u32 *reg)
{
    if (off < UFSHC_MCQ_REG_BASE ||
        off >= UFSHC_MCQ_REG_BASE + UFSHC_MAX_QNUM * UFSHC_MCQ_REG_STRIDE) {
        return false;
    }
    *qid = (off - UFSHC_MCQ_REG_BASE) / UFSHC_MCQ_REG_STRIDE;
    *reg = (off - UFSHC_MCQ_REG_BASE) % UFSHC_MCQ_REG_STRIDE;
    return true;
}

static bool ufshc_decode_mcq_op(u32 off, u32 *qid, u32 *reg)
{
    if (off < UFSHC_MCQ_OP_BASE ||
        off >= UFSHC_MCQ_OP_BASE + UFSHC_MAX_QNUM * UFSHC_MCQ_OP_STRIDE) {
        return false;
    }
    *qid = (off - UFSHC_MCQ_OP_BASE) / UFSHC_MCQ_OP_STRIDE;
    *reg = (off - UFSHC_MCQ_OP_BASE) % UFSHC_MCQ_OP_STRIDE;
    return true;
}

static u32 ufshc_read_mcq_cfg(ufshc_t *u, u32 qid, u32 reg)
{
    ufshc_mcq_cfg_t *c = &u->mcq_cfg[qid];
    switch (reg) {
    case UFSHC_MCQ_A_SQATTR: return c->sqattr;
    case UFSHC_MCQ_A_SQLBA: return c->sqlba;
    case UFSHC_MCQ_A_SQUBA: return c->squba;
    case UFSHC_MCQ_A_SQDAO: return c->sqdao;
    case UFSHC_MCQ_A_SQISAO: return c->sqisao;
    case UFSHC_MCQ_A_SQCFG: return c->sqcfg;
    case UFSHC_MCQ_A_CQATTR: return c->cqattr;
    case UFSHC_MCQ_A_CQLBA: return c->cqlba;
    case UFSHC_MCQ_A_CQUBA: return c->cquba;
    case UFSHC_MCQ_A_CQDAO: return c->cqdao;
    case UFSHC_MCQ_A_CQISAO: return c->cqisao;
    case UFSHC_MCQ_A_CQCFG: return c->cqcfg;
    default: return 0;
    }
}

static void ufshc_write_mcq_cfg(ufshc_t *u, u32 qid, u32 reg, u32 data)
{
    ufshc_mcq_cfg_t *c = &u->mcq_cfg[qid];
    switch (reg) {
    case UFSHC_MCQ_A_SQATTR: c->sqattr = data; break;
    case UFSHC_MCQ_A_SQLBA: c->sqlba = data; break;
    case UFSHC_MCQ_A_SQUBA: c->squba = data; break;
    case UFSHC_MCQ_A_SQDAO: c->sqdao = data; break;
    case UFSHC_MCQ_A_SQISAO: c->sqisao = data; break;
    case UFSHC_MCQ_A_SQCFG: c->sqcfg = data; break;
    case UFSHC_MCQ_A_CQATTR: c->cqattr = data; break;
    case UFSHC_MCQ_A_CQLBA: c->cqlba = data; break;
    case UFSHC_MCQ_A_CQUBA: c->cquba = data; break;
    case UFSHC_MCQ_A_CQDAO: c->cqdao = data; break;
    case UFSHC_MCQ_A_CQISAO: c->cqisao = data; break;
    case UFSHC_MCQ_A_CQCFG: c->cqcfg = data; break;
    default: break;
    }
    ufshc_update_queue_cfg(u, qid);
    for (u32 i = 0; i < UFSHC_MAX_QNUM; i++) {
        ufshc_update_queue_cfg(u, i);
    }
    if (reg == UFSHC_MCQ_A_SQATTR || reg == UFSHC_MCQ_A_CQATTR ||
        reg == UFSHC_MCQ_A_SQLBA || reg == UFSHC_MCQ_A_CQLBA ||
        reg == UFSHC_MCQ_A_SQDAO || reg == UFSHC_MCQ_A_SQISAO ||
        reg == UFSHC_MCQ_A_CQDAO || reg == UFSHC_MCQ_A_CQISAO) {
        UFSHC_LOG(u,
                  "q%u cfg reg=%02x data=%08x sq_en=%u cq_en=%u sq_base=%08x cq_base=%08x sq_depth=%u cq_depth=%u cqid=%u\n",
                  qid, reg, data, u->queues[qid].sq_en, u->queues[qid].cq_en,
                  u->queues[qid].sq_base, u->queues[qid].cq_base,
                  u->queues[qid].sq_depth, u->queues[qid].cq_depth,
                  u->queues[qid].cqid);
    }
}

static u32 ufshc_read_mcq_op(ufshc_t *u, u32 qid, u32 reg)
{
    ufshc_mcq_op_t *o = &u->mcq_op[qid];
    switch (reg) {
    case UFSHC_OP_A_SQHP: return o->sq_hp;
    case UFSHC_OP_A_SQTP: return o->sq_tp;
    case UFSHC_OP_A_SQRTC: return o->sq_rtc;
    case UFSHC_OP_A_SQCTI: return o->sq_cti;
    case UFSHC_OP_A_SQRTS: return o->sq_rts;
    case UFSHC_OP_A_SQIS: return o->sq_is;
    case UFSHC_OP_A_SQIE: return o->sq_ie;
    case UFSHC_OP_A_CQHP: return o->cq_hp;
    case UFSHC_OP_A_CQTP: return o->cq_tp;
    case UFSHC_OP_A_CQIS: return o->cq_is;
    case UFSHC_OP_A_CQIE: return o->cq_ie;
    case UFSHC_OP_A_CQIACR: return o->cq_iacr;
    default: return 0;
    }
}

static void ufshc_write_mcq_op(ufshc_t *u, u32 qid, u32 reg, u32 data)
{
    ufshc_mcq_op_t *o = &u->mcq_op[qid];
    switch (reg) {
    case UFSHC_OP_A_SQTP:
        o->sq_tp = data;
        UFSHC_LOG(u, "q%u SQTP=%08x SQHP=%08x\n", qid, o->sq_tp, o->sq_hp);
        break;
    case UFSHC_OP_A_CQHP:
        o->cq_hp = data;
        UFSHC_LOG(u, "q%u CQHP=%08x CQTP=%08x\n", qid, o->cq_hp, o->cq_tp);
        break;
    case UFSHC_OP_A_CQIS:
        o->cq_is &= ~data;
        UFSHC_LOG(u, "q%u clear CQIS mask=%08x now=%08x\n", qid, data, o->cq_is);
        break;
    case UFSHC_OP_A_CQIE:
        o->cq_ie = data;
        UFSHC_LOG(u, "q%u CQIE=%08x\n", qid, data);
        break;
    case UFSHC_OP_A_CQIACR:
        o->cq_iacr = data;
        break;
    case UFSHC_OP_A_SQIE:
        o->sq_ie = data;
        break;
    case UFSHC_OP_A_SQIS:
        o->sq_is &= ~data;
        break;
    default:
        break;
    }
}

static u32 ufshc_reg_read(ufshc_t *u, u32 off)
{
    u32 qid, reg;
    if (ufshc_decode_mcq_cfg(off, &qid, &reg)) {
        return ufshc_read_mcq_cfg(u, qid, reg);
    }
    if (ufshc_decode_mcq_op(off, &qid, &reg)) {
        return ufshc_read_mcq_op(u, qid, reg);
    }
    return ufshc_read_main_reg(u, off);
}

static void ufshc_reg_write(ufshc_t *u, u32 off, u32 data)
{
    u32 qid, reg;
    if (ufshc_decode_mcq_cfg(off, &qid, &reg)) {
        ufshc_write_mcq_cfg(u, qid, reg, data);
        return;
    }
    if (ufshc_decode_mcq_op(off, &qid, &reg)) {
        ufshc_write_mcq_op(u, qid, reg, data);
        return;
    }
    ufshc_write_main_reg(u, off, data);
}

static void ufshc_apb_proc(ufshc_t *u)
{
    if (itf_fifo_empty(u->apb_req_slv) || itf_fifo_full(u->apb_rsp_mst)) {
        return;
    }

    apb_req_if_t req;
    itf_read(u->apb_req_slv, &req);
    DBG_CHECK(req.paddr >= u->base_addr);
    DBG_CHECK(req.paddr < u->base_addr + u->size);

    u32 off = req.paddr - u->base_addr;
    apb_rsp_if_t rsp = {.prdata = 0, .pslverr = false};
    if (req.pwrite) {
        ufshc_reg_write(u, off, req.pwdata);
    } else {
        rsp.prdata = ufshc_reg_read(u, off);
    }
    itf_write(u->apb_rsp_mst, &rsp);
}

void ufshc_construct(ufshc_t *u, const char *parent, const char *name,
                     u32 base, u32 size, const ufshc_conf_t *conf)
{
    DBG_CHECK(conf != NULL);

    u->en = conf->en;
    u->backing_path = conf->backing_path;
    if (!u->en) {
        return;
    }

    mod_construct(&u->mod, parent, name);
    DBG_VCD_MODULE_SCOPE(name);

    u->base_addr = base;
    u->size = size;
    u->storage_size = 0;
    u->xfer_buf_size = UFSHC_MAX_XFER;
    u->backing_fp = NULL;
    u->log_enable = dbg_get_bool_env("UFSHC_LOG");
    u->xfer_buf = malloc(u->xfer_buf_size);
    DBG_CHECK(u->xfer_buf != NULL);
    DBG_CHECK(ufshc_open_backing(u));
    u->irq_o = itf_signal_get_src_and_chk(u->irq_out);

    dbg_vcd_add_sig("is", DBG_SIG_TYPE_REG, 32, &u->is);
    dbg_vcd_add_sig("ie", DBG_SIG_TYPE_REG, 32, &u->ie);
    dbg_vcd_add_sig("hcs", DBG_SIG_TYPE_REG, 32, &u->hcs);
    dbg_vcd_add_sig("req_state", DBG_SIG_TYPE_REG, 4, &u->req_state);
}

void ufshc_reset(ufshc_t *u)
{
    if (!u->en) {
        return;
    }

    mod_reset(&u->mod);

    memset(u->xfer_buf, 0, u->xfer_buf_size);
    memset(&u->mcq_cfg, 0, sizeof(u->mcq_cfg));
    memset(&u->mcq_op, 0, sizeof(u->mcq_op));
    memset(&u->queues, 0, sizeof(u->queues));
    memset(&u->dma, 0, sizeof(u->dma));
    memset(&u->req, 0, sizeof(u->req));

    u->cap = ((UFSHC_MAX_QNUM - 1u) << UFSHC_CAP_NUTRS_SHIFT) |
             (2u << UFSHC_CAP_RTT_SHIFT) |
             (0u << UFSHC_CAP_NUTMRS_SHIFT) |
             UFSHC_CAP_64AS | UFSHC_CAP_MCQS;
    u->mcqcap = ((UFSHC_MAX_QNUM - 1u) << UFSHC_MCQCAP_MAXQ_SHIFT) |
                UFSHC_MCQCAP_RRP |
                (UFSHC_MCQ_QCFGPTR << UFSHC_MCQCAP_QCFGPTR_SHIFT);
    u->ver = UFSHC_SPEC_VER;
    u->hcpid = 0;
    u->hcmid = 0;
    u->ahit = 0;
    u->is = 0;
    u->ie = 0;
    u->hcs = 0;
    u->hce = 0;
    u->uecpa = 0;
    u->uecdl = 0;
    u->uecn = 0;
    u->uect = 0;
    u->uecdme = 0;
    u->utriacr = 0;
    u->utrlba = 0;
    u->utrlbau = 0;
    u->utrldbr = 0;
    u->utrlclr = 0;
    u->utrlrsr = 0;
    u->utrlcnr = 0;
    u->utmrlba = 0;
    u->utmrlbau = 0;
    u->utmrlrsr = 0;
    u->utmrlclr = 0;
    u->utmrldbr = 0;
    u->uiccmd = 0;
    u->ucmdarg1 = 0;
    u->ucmdarg2 = 0;
    u->ucmdarg3 = 0;
    u->ccap = 0;
    u->config = 0;
    u->mcqconfig = 0x1fu << UFSHC_MCQCONFIG_MAC_SHIFT;
    u->esilba = 0;
    u->esiuba = 0;
    for (u32 qid = 0; qid < UFSHC_MAX_QNUM; qid++) {
        u32 op_base = mcq_op_addr(qid);
        u->mcq_cfg[qid].sqdao = op_base + UFSHC_OP_A_SQHP;
        u->mcq_cfg[qid].sqisao = op_base + UFSHC_OP_A_SQIS;
        u->mcq_cfg[qid].cqdao = op_base + UFSHC_OP_A_CQHP;
        u->mcq_cfg[qid].cqisao = op_base + UFSHC_OP_A_CQIS;
    }
    u->req_state = UFSHC_REQ_IDLE;
    u->dma.state = UFSHC_DMA_IDLE;

    for (u32 i = 0; i < UFSHC_MAX_QNUM; i++) {
        u32 op = mcq_op_addr(i);
        u->mcq_cfg[i].sqdao = op + UFSHC_OP_A_SQHP;
        u->mcq_cfg[i].sqisao = op + UFSHC_OP_A_SQIS;
        u->mcq_cfg[i].cqdao = op + UFSHC_OP_A_CQHP;
        u->mcq_cfg[i].cqisao = op + UFSHC_OP_A_CQIS;
    }

    ufshc_init_descs(u);
    u->irq_o->irq = false;
    itf_signal_write_notify(u->irq_out);
}

void ufshc_clock(ufshc_t *u)
{
    if (!u->en) {
        return;
    }

    mod_clock(&u->mod);
    ufshc_dma_clock(u);
    ufshc_req_clock(u);
    ufshc_apb_proc(u);
}

void ufshc_free(ufshc_t *u)
{
    if (!u->en) {
        return;
    }

    mod_free(&u->mod);
    if (u->backing_fp != NULL) {
        fflush(u->backing_fp);
        fclose(u->backing_fp);
        u->backing_fp = NULL;
    }
    free(u->xfer_buf);
}
