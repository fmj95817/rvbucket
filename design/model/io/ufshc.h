#ifndef IO_UFSHC_H
#define IO_UFSHC_H

#include <stdio.h>
#include "base/types.h"
#include "base/mod.h"
#include "base/itf.h"
#include "itf/apb_if.h"
#include "itf/axi4_if.h"
#include "itf/ext_irq_if.h"
#include "spec/io/ufshc.h"

typedef struct __attribute__((packed)) ufshc_upiu_header {
    u8 trans_type;
    u8 flags;
    u8 lun;
    u8 task_tag;
    u8 iid_cmd_set_type;
    u8 query_func;
    u8 response;
    u8 scsi_status;
    u8 ehs_len;
    u8 device_inf;
    u16 data_segment_length;
} ufshc_upiu_header_t;

typedef struct __attribute__((packed)) ufshc_upiu_cmd {
    u32 exp_data_transfer_len;
    u8 cdb[UFSHC_CDB_SIZE];
} ufshc_upiu_cmd_t;

typedef struct __attribute__((packed)) ufshc_upiu_query {
    u8 opcode;
    u8 idn;
    u8 index;
    u8 selector;
    u16 reserved_osf;
    u16 length;
    u32 value;
    u32 reserved[2];
    u8 data[UFSHC_MAX_QUERY_DATA_SIZE];
} ufshc_upiu_query_t;

typedef struct __attribute__((packed)) ufshc_upiu_req {
    ufshc_upiu_header_t header;
    union {
        ufshc_upiu_cmd_t sc;
        ufshc_upiu_query_t qr;
    };
} ufshc_upiu_req_t;

typedef struct __attribute__((packed)) ufshc_upiu_scsi_rsp {
    u32 residual_transfer_count;
    u32 reserved[4];
    u16 sense_data_len;
    u8 sense_data[UFSHC_SENSE_SIZE];
} ufshc_upiu_scsi_rsp_t;

typedef struct __attribute__((packed)) ufshc_upiu_rsp {
    ufshc_upiu_header_t header;
    union {
        ufshc_upiu_scsi_rsp_t sr;
        ufshc_upiu_query_t qr;
    };
} ufshc_upiu_rsp_t;

typedef struct __attribute__((packed)) ufshc_utrd {
    u32 dword_0;
    u32 dword_1;
    u32 dword_2;
    u32 dword_3;
    u32 command_desc_base_addr_lo;
    u32 command_desc_base_addr_hi;
    u16 response_upiu_length;
    u16 response_upiu_offset;
    u16 prd_table_length;
    u16 prd_table_offset;
} ufshc_utrd_t;

typedef ufshc_utrd_t ufshc_sqe_t;

typedef struct __attribute__((packed)) ufshc_prdt {
    u64 addr;
    u32 reserved;
    u32 size;
} ufshc_prdt_t;

typedef struct __attribute__((packed)) ufshc_cqe {
    u64 utp_addr;
    u16 resp_len;
    u16 resp_off;
    u16 prdt_len;
    u16 prdt_off;
    u8 status;
    u8 error;
    u16 rsvd1;
    u32 rsvd2[3];
} ufshc_cqe_t;

typedef enum ufshc_dma_state {
    UFSHC_DMA_IDLE = 0,
    UFSHC_DMA_RD_SEND,
    UFSHC_DMA_RD_WAIT,
    UFSHC_DMA_WR_SEND,
    UFSHC_DMA_WR_WAIT,
} ufshc_dma_state_t;

typedef struct ufshc_dma {
    ufshc_dma_state_t state;
    bool write;
    bool ok;
    u32 addr;
    u8 *buf;
    u32 len;
    u32 pos;
    u32 beat_addr;
    u8 beat_size;
    u8 beat_bytes;
} ufshc_dma_t;

typedef struct ufshc_conf {
    bool en;
    const char *backing_path;
} ufshc_conf_t;

typedef enum ufshc_req_state {
    UFSHC_REQ_IDLE = 0,
    UFSHC_REQ_READ_SQE,
    UFSHC_REQ_READ_UPIU,
    UFSHC_REQ_READ_PRDT,
    UFSHC_REQ_EXEC,
    UFSHC_REQ_PRDT_DATA,
    UFSHC_REQ_WRITE_RSP,
    UFSHC_REQ_WRITE_CQE,
} ufshc_req_state_t;

typedef enum ufshc_data_dir {
    UFSHC_DATA_NONE = 0,
    UFSHC_DATA_TO_HOST,
    UFSHC_DATA_FROM_HOST,
} ufshc_data_dir_t;

typedef struct ufshc_mcq_cfg {
    u32 sqattr;
    u32 sqlba;
    u32 squba;
    u32 sqdao;
    u32 sqisao;
    u32 sqcfg;
    u32 cqattr;
    u32 cqlba;
    u32 cquba;
    u32 cqdao;
    u32 cqisao;
    u32 cqcfg;
} ufshc_mcq_cfg_t;

typedef struct ufshc_mcq_op {
    u32 sq_hp;
    u32 sq_tp;
    u32 sq_rtc;
    u32 sq_cti;
    u32 sq_rts;
    u32 sq_is;
    u32 sq_ie;
    u32 cq_hp;
    u32 cq_tp;
    u32 cq_is;
    u32 cq_ie;
    u32 cq_iacr;
} ufshc_mcq_op_t;

typedef struct ufshc_queue {
    bool sq_en;
    bool cq_en;
    u32 sq_base;
    u32 cq_base;
    u32 sq_depth;
    u32 cq_depth;
    u8 cqid;
} ufshc_queue_t;

typedef struct ufshc_active_req {
    u8 qid;
    ufshc_utrd_t utrd;
    ufshc_upiu_req_t req_upiu;
    ufshc_upiu_rsp_t rsp_upiu;
    ufshc_prdt_t prdt[UFSHC_MAX_PRDT];
    ufshc_cqe_t cqe;
    u32 cmd_desc_base;
    u32 rsp_upiu_addr;
    u32 rsp_upiu_len;
    u32 prdt_base;
    u32 prdt_len;
    u32 req_result;
    u32 rsp_write_len;
    ufshc_data_dir_t data_dir;
    u32 data_len;
    u32 data_lba;
    u32 prdt_idx;
    u32 prdt_off;
    u32 data_pos;
    bool data_commit_pending;
} ufshc_active_req_t;

typedef struct __attribute__((packed)) ufshc_image_header {
    u8 magic[UFSHC_IMAGE_MAGIC_SIZE];
    u32 version;
    u32 header_size;
    u64 storage_size;
    u32 block_size;
    u32 flags;
    u8 reserved[UFSHC_IMAGE_HEADER_SIZE - UFSHC_IMAGE_MAGIC_SIZE -
                sizeof(u32) - sizeof(u32) - sizeof(u64) -
                sizeof(u32) - sizeof(u32)];
} ufshc_image_header_t;

_Static_assert(sizeof(ufshc_image_header_t) == UFSHC_IMAGE_HEADER_SIZE,
    "UFS image header must occupy exactly one UFS block");

typedef struct ufshc {
    mod_t mod;
    itf_t *apb_req_slv;
    itf_t *apb_rsp_mst;
    AXI4_MST_DECL(dma_);
    itf_t *irq_out;

    u32 base_addr;
    u32 size;
    bool en;
    ext_irq_if_t *irq_o;

    u32 cap;
    u32 mcqcap;
    u32 ver;
    u32 hcpid;
    u32 hcmid;
    u32 ahit;
    u32 is;
    u32 ie;
    u32 hcs;
    u32 hce;
    u32 uecpa;
    u32 uecdl;
    u32 uecn;
    u32 uect;
    u32 uecdme;
    u32 utriacr;
    u32 utrlba;
    u32 utrlbau;
    u32 utrldbr;
    u32 utrlclr;
    u32 utrlrsr;
    u32 utrlcnr;
    u32 utmrlba;
    u32 utmrlbau;
    u32 utmrldbr;
    u32 utmrlclr;
    u32 utmrlrsr;
    u32 uiccmd;
    u32 ucmdarg1;
    u32 ucmdarg2;
    u32 ucmdarg3;
    u32 ccap;
    u32 config;
    u32 mcqconfig;
    u32 esilba;
    u32 esiuba;

    ufshc_mcq_cfg_t mcq_cfg[UFSHC_MAX_QNUM];
    ufshc_mcq_op_t mcq_op[UFSHC_MAX_QNUM];
    ufshc_queue_t queues[UFSHC_MAX_QNUM];

    ufshc_dma_t dma;
    ufshc_req_state_t req_state;
    ufshc_active_req_t req;
    u32 storage_size;
    u8 *xfer_buf;
    u32 xfer_buf_size;
    const char *backing_path;
    FILE *backing_fp;
    bool log_enable;

    u8 device_desc[89];
    u8 unit_desc[45];
    u8 geometry_desc[87];
    u8 flags[UFSHC_FLAG_IDN_COUNT];
    u32 attrs[UFSHC_ATTR_IDN_COUNT];
} ufshc_t;

void ufshc_construct(ufshc_t *u, const char *parent, const char *name,
                     u32 base, u32 size, const ufshc_conf_t *conf);
void ufshc_reset(ufshc_t *u);
void ufshc_clock(ufshc_t *u);
void ufshc_free(ufshc_t *u);

#endif
