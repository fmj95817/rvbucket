#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define UFS_BASE        0x30003000u
#define DDR_BASE        0x40000000u
#define UFS_DMA_POOL_BASE 0x4f000000u
#define SQ_BASE         (UFS_DMA_POOL_BASE + 0x0000u)
#define CQ_BASE         (UFS_DMA_POOL_BASE + 0x1000u)
#define CMD_BASE        (UFS_DMA_POOL_BASE + 0x2000u)
#define DATA_BASE       0x40110000u
#define CBO_BLOCK_SIZE  64u
#define QUEUE_DEPTH     4u
#define CMD_STRIDE      0x3000u
#define RSP_OFF         1024u
#define PRDT_OFF        2048u
#define BLOCK_SIZE      4096u

#define UFS_A_CAP       0x000u
#define UFS_A_MCQCAP    0x004u
#define UFS_A_IS        0x020u
#define UFS_A_IE        0x024u
#define UFS_A_HCS       0x030u
#define UFS_A_HCE       0x034u
#define UFS_A_UICCMD    0x090u

#define UFS_MCQ_REG_BASE   0x400u
#define UFS_MCQ_OP_BASE    0x1000u
#define UFS_MCQ_A_SQATTR   0x00u
#define UFS_MCQ_A_SQLBA    0x04u
#define UFS_MCQ_A_CQATTR   0x20u
#define UFS_MCQ_A_CQLBA    0x24u
#define UFS_OP_A_SQTP      0x04u
#define UFS_OP_A_CQHP      0x1cu
#define UFS_OP_A_CQIS      0x24u

#define UFS_CAP_MCQS       (1u << 30)
#define UFS_IS_UCCS        (1u << 10)
#define UFS_IS_CQES        (1u << 20)
#define UFS_HCS_DP         (1u << 0)
#define UFS_HCS_UCRDY      (1u << 3)
#define UFS_HCE_HCE        (1u << 0)
#define UFS_SQATTR_SQEN    (1u << 31)
#define UFS_CQATTR_CQEN    (1u << 31)
#define UFS_CQIS_TEPS      (1u << 0)
#define UFS_UIC_LINK_STARTUP 0x16u

#define UPIU_NOP_OUT       0x00u
#define UPIU_COMMAND       0x01u
#define UPIU_QUERY_REQ     0x16u
#define UPIU_NOP_IN        0x20u
#define UPIU_RESPONSE      0x21u
#define UPIU_QUERY_RSP     0x36u
#define UPIU_FLAG_READ     0x40u
#define UPIU_FLAG_WRITE    0x20u
#define UPIU_QUERY_READ    0x01u

#define QUERY_READ_DESC    0x01u
#define DESC_DEVICE        0x00u

#define SCSI_WRITE_10      0x2au
#define SCSI_READ_10       0x28u

#define OCS_SUCCESS        0x00u

typedef struct __attribute__((packed)) upiu_header {
    uint8_t trans_type;
    uint8_t flags;
    uint8_t lun;
    uint8_t task_tag;
    uint8_t iid_cmd_set_type;
    uint8_t query_func;
    uint8_t response;
    uint8_t scsi_status;
    uint8_t ehs_len;
    uint8_t device_inf;
    uint16_t data_segment_length;
} upiu_header_t;

typedef struct __attribute__((packed)) upiu_cmd {
    uint32_t exp_data_transfer_len;
    uint8_t cdb[16];
} upiu_cmd_t;

typedef struct __attribute__((packed)) upiu_query {
    uint8_t opcode;
    uint8_t idn;
    uint8_t index;
    uint8_t selector;
    uint16_t reserved_osf;
    uint16_t length;
    uint32_t value;
    uint32_t reserved[2];
    uint8_t data[256];
} upiu_query_t;

typedef struct __attribute__((packed)) upiu_req {
    upiu_header_t header;
    union {
        upiu_cmd_t sc;
        upiu_query_t qr;
    };
} upiu_req_t;

typedef struct __attribute__((packed)) upiu_rsp {
    upiu_header_t header;
    union {
        struct {
            uint32_t residual_transfer_count;
            uint32_t reserved[4];
            uint16_t sense_data_len;
            uint8_t sense_data[18];
        } sr;
        upiu_query_t qr;
    };
} upiu_rsp_t;

typedef struct __attribute__((packed)) utrd {
    uint32_t dword_0;
    uint32_t dword_1;
    uint32_t dword_2;
    uint32_t dword_3;
    uint32_t command_desc_base_addr_lo;
    uint32_t command_desc_base_addr_hi;
    uint16_t response_upiu_length;
    uint16_t response_upiu_offset;
    uint16_t prd_table_length;
    uint16_t prd_table_offset;
} utrd_t;

typedef struct __attribute__((packed)) prdt {
    uint64_t addr;
    uint32_t reserved;
    uint32_t size;
} prdt_t;

typedef struct __attribute__((packed)) cqe {
    uint64_t utp_addr;
    uint16_t resp_len;
    uint16_t resp_off;
    uint16_t prdt_len;
    uint16_t prdt_off;
    uint8_t status;
    uint8_t error;
    uint16_t rsvd1;
    uint32_t rsvd2[3];
} cqe_t;

static volatile uint32_t *const ufs = (volatile uint32_t *)UFS_BASE;
static uint32_t sq_tail;
static uint32_t cq_head;

static inline void cbo_inval(void *addr)
{
    asm volatile(".insn i 0x0f, 2, x0, %0, 0" :: "r"(addr) : "memory");
}

static inline void cbo_clean(void *addr)
{
    asm volatile(".insn i 0x0f, 2, x0, %0, 1" :: "r"(addr) : "memory");
}

static inline void cbo_flush(void *addr)
{
    asm volatile(".insn i 0x0f, 2, x0, %0, 2" :: "r"(addr) : "memory");
}

static void cbo_range(void *addr, uint32_t len, void (*op)(void *))
{
    uintptr_t start = (uintptr_t)addr & ~(uintptr_t)(CBO_BLOCK_SIZE - 1u);
    uintptr_t end = ((uintptr_t)addr + len + CBO_BLOCK_SIZE - 1u) &
        ~(uintptr_t)(CBO_BLOCK_SIZE - 1u);
    for (uintptr_t p = start; p < end; p += CBO_BLOCK_SIZE) {
        op((void *)p);
    }
    asm volatile("fence rw, rw" ::: "memory");
}

static uint16_t be16(uint16_t v)
{
    return (uint16_t)((v >> 8) | (v << 8));
}

static uint32_t be32(uint32_t v)
{
    return ((v >> 24) & 0x000000ffu) |
           ((v >> 8)  & 0x0000ff00u) |
           ((v << 8)  & 0x00ff0000u) |
           ((v << 24) & 0xff000000u);
}

static void writel(uint32_t off, uint32_t val)
{
    ufs[off / 4] = val;
}

static uint32_t readl(uint32_t off)
{
    return ufs[off / 4];
}

static bool wait_set(uint32_t off, uint32_t mask, uint32_t timeout)
{
    while (timeout--) {
        if (readl(off) & mask) {
            return true;
        }
    }
    return false;
}

static void clear_completion(void)
{
    cq_head = (cq_head + sizeof(cqe_t)) % (QUEUE_DEPTH * sizeof(cqe_t));
    writel(UFS_MCQ_OP_BASE + UFS_OP_A_CQHP, cq_head);
    writel(UFS_MCQ_OP_BASE + UFS_OP_A_CQIS, UFS_CQIS_TEPS);
    writel(UFS_A_IS, UFS_IS_CQES);
}

static void config_mcq(void)
{
    uint32_t qbytes = QUEUE_DEPTH * sizeof(utrd_t);
    uint32_t attr_size = qbytes / 4 - 1;

    writel(UFS_A_HCE, UFS_HCE_HCE);
    if (!wait_set(UFS_A_HCS, UFS_HCS_UCRDY, 100000)) {
        printf("ufshc_mcq: HCE timeout\n");
        return;
    }

    writel(UFS_A_UICCMD, UFS_UIC_LINK_STARTUP);
    if (!wait_set(UFS_A_IS, UFS_IS_UCCS, 100000) ||
        !(readl(UFS_A_HCS) & UFS_HCS_DP)) {
        printf("ufshc_mcq: link startup timeout\n");
        return;
    }
    writel(UFS_A_IS, 0xffffffffu);
    writel(UFS_A_IE, UFS_IS_CQES);

    writel(UFS_MCQ_REG_BASE + UFS_MCQ_A_CQLBA, CQ_BASE);
    writel(UFS_MCQ_REG_BASE + UFS_MCQ_A_CQATTR, attr_size | UFS_CQATTR_CQEN);
    writel(UFS_MCQ_REG_BASE + UFS_MCQ_A_SQLBA, SQ_BASE);
    writel(UFS_MCQ_REG_BASE + UFS_MCQ_A_SQATTR, attr_size | UFS_SQATTR_SQEN);
}

static cqe_t *submit_cmd(uint32_t cmd_addr, uint32_t data_addr, uint32_t data_len)
{
    volatile utrd_t *sqe = (volatile utrd_t *)(SQ_BASE + sq_tail);
    memset((void *)sqe, 0, sizeof(*sqe));
    sqe->dword_0 = 1u << 28;
    sqe->dword_2 = 0xf;
    sqe->command_desc_base_addr_lo = cmd_addr;
    sqe->response_upiu_offset = RSP_OFF / 4;
    sqe->response_upiu_length = sizeof(upiu_rsp_t) / 4;
    sqe->prd_table_offset = PRDT_OFF / 4;
    sqe->prd_table_length = data_len ? 1 : 0;

    if (data_len) {
        prdt_t *p = (prdt_t *)(cmd_addr + PRDT_OFF);
        memset(p, 0, sizeof(*p));
        p->addr = data_addr;
        p->size = data_len - 1;
    }

    asm volatile("fence iorw, iorw" ::: "memory");
    sq_tail = (sq_tail + sizeof(utrd_t)) % (QUEUE_DEPTH * sizeof(utrd_t));
    writel(UFS_MCQ_OP_BASE + UFS_OP_A_SQTP, sq_tail);

    if (!wait_set(UFS_A_IS, UFS_IS_CQES, 2000000)) {
        return NULL;
    }
    return (cqe_t *)(CQ_BASE + cq_head);
}

static bool test_nop(void)
{
    uint32_t cmd = CMD_BASE;
    upiu_req_t *req = (upiu_req_t *)cmd;
    memset(req, 0, sizeof(*req));
    req->header.trans_type = UPIU_NOP_OUT;
    req->header.task_tag = 1;

    cqe_t *cqe = submit_cmd(cmd, 0, 0);
    upiu_rsp_t *rsp = (upiu_rsp_t *)(cmd + RSP_OFF);
    if (!cqe || cqe->status != OCS_SUCCESS ||
        rsp->header.trans_type != UPIU_NOP_IN) {
        printf("ufshc_mcq: NOP failed cqe=%p status=%u rsp=0x%02x\n",
            (void *)cqe, cqe ? cqe->status : 0xff, rsp->header.trans_type);
        return false;
    }
    clear_completion();
    return true;
}

static bool test_query(void)
{
    uint32_t cmd = CMD_BASE + CMD_STRIDE;
    upiu_req_t *req = (upiu_req_t *)cmd;
    memset(req, 0, sizeof(*req));
    req->header.trans_type = UPIU_QUERY_REQ;
    req->header.query_func = UPIU_QUERY_READ;
    req->header.task_tag = 2;
    req->qr.opcode = QUERY_READ_DESC;
    req->qr.idn = DESC_DEVICE;
    req->qr.length = be16(89);

    cqe_t *cqe = submit_cmd(cmd, 0, 0);
    upiu_rsp_t *rsp = (upiu_rsp_t *)(cmd + RSP_OFF);
    if (!cqe || cqe->status != OCS_SUCCESS ||
        rsp->header.trans_type != UPIU_QUERY_RSP ||
        rsp->header.response != 0 ||
        rsp->qr.data[0] != 89 ||
        rsp->qr.data[6] != 1) {
        printf("ufshc_mcq: QUERY failed status=%u rsp=0x%02x response=0x%02x len=%u lu=%u\n",
            cqe ? cqe->status : 0xff, rsp->header.trans_type,
            rsp->header.response, rsp->qr.data[0], rsp->qr.data[6]);
        return false;
    }
    clear_completion();
    return true;
}

static bool test_scsi_rw(void)
{
    uint32_t write_cmd = CMD_BASE + 2 * CMD_STRIDE;
    uint32_t read_cmd = CMD_BASE + 3 * CMD_STRIDE;
    uint8_t *wbuf = (uint8_t *)DATA_BASE;
    uint8_t *rbuf = (uint8_t *)(DATA_BASE + 0x4000);

    for (uint32_t i = 0; i < BLOCK_SIZE; i++) {
        wbuf[i] = (uint8_t)(i ^ 0x5a);
        rbuf[i] = 0;
    }
    cbo_range(wbuf, BLOCK_SIZE, cbo_clean);

    upiu_req_t *req = (upiu_req_t *)write_cmd;
    memset(req, 0, sizeof(*req));
    req->header.trans_type = UPIU_COMMAND;
    req->header.flags = UPIU_FLAG_WRITE;
    req->header.task_tag = 3;
    req->sc.exp_data_transfer_len = be32(BLOCK_SIZE);
    req->sc.cdb[0] = SCSI_WRITE_10;
    req->sc.cdb[5] = 7;
    req->sc.cdb[8] = 1;
    cqe_t *cqe = submit_cmd(write_cmd, (uint32_t)wbuf, BLOCK_SIZE);
    if (!cqe || cqe->status != OCS_SUCCESS) {
        printf("ufshc_mcq: WRITE failed status=%u\n", cqe ? cqe->status : 0xff);
        return false;
    }
    clear_completion();

    cbo_range(rbuf, BLOCK_SIZE, cbo_flush);

    req = (upiu_req_t *)read_cmd;
    memset(req, 0, sizeof(*req));
    req->header.trans_type = UPIU_COMMAND;
    req->header.flags = UPIU_FLAG_READ;
    req->header.task_tag = 4;
    req->sc.exp_data_transfer_len = be32(BLOCK_SIZE);
    req->sc.cdb[0] = SCSI_READ_10;
    req->sc.cdb[5] = 7;
    req->sc.cdb[8] = 1;
    cqe = submit_cmd(read_cmd, (uint32_t)rbuf, BLOCK_SIZE);
    if (!cqe || cqe->status != OCS_SUCCESS) {
        printf("ufshc_mcq: READ failed status=%u\n", cqe ? cqe->status : 0xff);
        return false;
    }
    cbo_range(rbuf, BLOCK_SIZE, cbo_inval);
    for (uint32_t i = 0; i < BLOCK_SIZE; i++) {
        if (rbuf[i] != wbuf[i]) {
            printf("ufshc_mcq: data mismatch byte %u got=0x%02x exp=0x%02x\n",
                (unsigned int)i, rbuf[i], wbuf[i]);
            return false;
        }
    }
    clear_completion();
    return true;
}

int main(void)
{
    sq_tail = 0;
    cq_head = 0;

    if (!(readl(UFS_A_CAP) & UFS_CAP_MCQS)) {
        printf("ufshc_mcq: FAIL no MCQ capability cap=0x%08x mcqcap=0x%08x\n",
            (unsigned int)readl(UFS_A_CAP),
            (unsigned int)readl(UFS_A_MCQCAP));
        return 1;
    }

    config_mcq();
    if (!test_nop() || !test_query() || !test_scsi_rw()) {
        printf("ufshc_mcq: FAIL\n");
        return 1;
    }

    printf("ufshc_mcq: PASS\n");
    return 0;
}
