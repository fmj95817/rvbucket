#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "io/ufshc.h"
#include "mem/ram.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_UFS_BASE     0x10000000u
#define TB_DDR_BASE     0x40000000u
#define TB_DDR_SIZE     (2u * MiB)
#define TB_SQ_BASE      (TB_DDR_BASE + 0x1000u)
#define TB_CQ_BASE      (TB_DDR_BASE + 0x2000u)
#define TB_CMD_BASE     (TB_DDR_BASE + 0x4000u)
#define TB_DATA_BASE    (TB_DDR_BASE + 0x10000u)
#define TB_QUEUE_DEPTH  4u
#define TB_CMD_STRIDE   0x3000u
#define TB_RSP_OFF      1024u
#define TB_PRDT_OFF     2048u

static inline u16 tb_be16(u16 v)
{
    return (u16)((v >> 8) | (v << 8));
}

static inline u32 tb_be32(u32 v)
{
    return ((v >> 24) & 0x000000ffu) |
           ((v >> 8)  & 0x0000ff00u) |
           ((v << 8)  & 0x00ff0000u) |
           ((v << 24) & 0xff000000u);
}

static inline u64 tb_be64(u64 v)
{
    return ((u64)tb_be32((u32)v) << 32) | tb_be32((u32)(v >> 32));
}

static inline u8 *tb_mem_ptr(ram_t *ram, u32 addr)
{
    return ram->data + (addr - TB_DDR_BASE);
}

typedef struct ufshc_tb {
    mod_t mod;
    u64 cycle_val;
    itf_t apb_req_itf;
    itf_t apb_rsp_itf;
    AXI4_IF_DECL(dma_);
    itf_t irq_itf;
    ext_irq_if_t *irq_o;
    ufshc_t dut;
    ram_t ram;
    char image_path[256];
    u32 next_sq_tail;
    u32 next_cq_head;
    ut_sbd_t sbd;
} ufshc_tb_t;

static void tb_construct(ufshc_tb_t *tb, const char *name)
{
    tb->cycle_val = 0;
    tb->mod.cycle = &tb->cycle_val;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    snprintf(tb->image_path, sizeof(tb->image_path),
             "/tmp/rvbucket_ufshc_%ld_XXXXXX", (long)getpid());
    int image_fd = mkstemp(tb->image_path);
    DBG_CHECK(image_fd >= 0);
    close(image_fd);

    APB_REQ_IF_CONSTRUCT(tb, apb_req_itf, 4);
    APB_RSP_IF_CONSTRUCT(tb, apb_rsp_itf, 4);
    AXI4_IF_CONSTRUCT(tb, dma_, 16);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, irq_itf, true, false);
    tb->irq_o = itf_signal_get_src_and_chk(&tb->irq_itf);

    tb->dut.apb_req_slv = &tb->apb_req_itf;
    tb->dut.apb_rsp_mst = &tb->apb_rsp_itf;
    AXI4_MST_CONNECT(&tb->dut, dma_, tb, dma_);
    tb->dut.irq_out = &tb->irq_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    ufshc_conf_t conf = {
        .en = true,
        .backing_path = tb->image_path
    };
    ufshc_construct(&tb->dut, tb->mod.hier_name, "u_dut",
                    TB_UFS_BASE, UFSHC_REG_SIZE, &conf);

    AXI4_SLV_CONNECT(&tb->ram, , tb, dma_);
    tb->ram.mod.cycle = tb->mod.cycle;
    ram_construct(&tb->ram, tb->mod.hier_name, "u_ram", 1, RAM_MODE_AXI,
                  TB_DDR_SIZE, TB_DDR_BASE, 0);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(ufshc_tb_t *tb)
{
    ufshc_reset(&tb->dut);
    ram_reset(&tb->ram);
    itf_reset(&tb->apb_req_itf);
    itf_reset(&tb->apb_rsp_itf);
    AXI4_IF_RESET(tb, dma_);
    itf_reset(&tb->irq_itf);
    tb->next_sq_tail = 0;
    tb->next_cq_head = 0;
    dbg_vcd_reset();
}

static void tb_free(ufshc_tb_t *tb)
{
    ufshc_free(&tb->dut);
    ram_free(&tb->ram);
    itf_free(&tb->apb_req_itf);
    itf_free(&tb->apb_rsp_itf);
    AXI4_IF_FREE(tb, dma_);
    itf_free(&tb->irq_itf);
    unlink(tb->image_path);
    mod_free(&tb->mod);
}

static void tb_clock(ufshc_tb_t *tb)
{
    ufshc_clock(&tb->dut);
    ram_clock(&tb->ram);
    itf_dbg_clock(&tb->apb_req_itf);
    itf_dbg_clock(&tb->apb_rsp_itf);
    AXI4_IF_DBG_CLOCK(tb, dma_);
    itf_dbg_clock(&tb->irq_itf);
    tb->cycle_val++;
    dbg_vcd_clock();
}

static bool tb_cond_apb_rsp(ufshc_tb_t *tb)
{
    return !itf_fifo_empty(&tb->apb_rsp_itf);
}

static bool tb_cond_irq(ufshc_tb_t *tb)
{
    return tb->irq_o->irq;
}

static void tb_apb_write(ufshc_tb_t *tb, u32 off, u32 data)
{
    apb_req_if_t req = {
        .paddr = TB_UFS_BASE + off,
        .pwrite = true,
        .pwdata = data,
        .pstrb = 0xf
    };
    itf_write(&tb->apb_req_itf, &req);
    RUN_POLL_UNTIL(tb_cond_apb_rsp, UT_TIMEOUT);
    apb_rsp_if_t rsp;
    itf_read(&tb->apb_rsp_itf, &rsp);
}

static u32 tb_apb_read(ufshc_tb_t *tb, u32 off)
{
    apb_req_if_t req = {
        .paddr = TB_UFS_BASE + off,
        .pwrite = false,
        .pwdata = 0,
        .pstrb = 0xf
    };
    itf_write(&tb->apb_req_itf, &req);
    RUN_POLL_UNTIL(tb_cond_apb_rsp, UT_TIMEOUT);
    apb_rsp_if_t rsp;
    itf_read(&tb->apb_rsp_itf, &rsp);
    return rsp.prdata;
}

static void tb_config_mcq(ufshc_tb_t *tb)
{
    u32 qbytes = TB_QUEUE_DEPTH * UFSHC_SQ_ENTRY_SIZE;
    u32 attr_size = (qbytes / 4u) - 1u;

    tb_apb_write(tb, UFSHC_A_HCE, UFSHC_HCE_HCE);
    tb_apb_write(tb, UFSHC_A_UICCMD, UFSHC_UIC_CMD_DME_LINK_STARTUP);
    tb_apb_write(tb, UFSHC_A_IS, 0xffffffffu);
    tb_apb_write(tb, UFSHC_A_IE, UFSHC_IS_CQES);

    tb_apb_write(tb, UFSHC_MCQ_REG_BASE + UFSHC_MCQ_A_CQLBA, TB_CQ_BASE);
    tb_apb_write(tb, UFSHC_MCQ_REG_BASE + UFSHC_MCQ_A_CQATTR,
                 attr_size | UFSHC_CQATTR_CQEN);
    tb_apb_write(tb, UFSHC_MCQ_REG_BASE + UFSHC_MCQ_A_SQLBA, TB_SQ_BASE);
    tb_apb_write(tb, UFSHC_MCQ_REG_BASE + UFSHC_MCQ_A_SQATTR,
                 attr_size | UFSHC_SQATTR_SQEN);
}

static u32 tb_submit_cmd(ufshc_tb_t *tb, u32 cmd_addr)
{
    ufshc_utrd_t sqe;
    memset(&sqe, 0, sizeof(sqe));
    sqe.dword_0 = 1u << 28;
    sqe.dword_2 = UFSHC_OCS_INVALID_COMMAND;
    sqe.command_desc_base_addr_lo = cmd_addr;
    sqe.response_upiu_offset = TB_RSP_OFF / sizeof(u32);
    sqe.response_upiu_length = sizeof(ufshc_upiu_rsp_t) / sizeof(u32);
    sqe.prd_table_offset = TB_PRDT_OFF / sizeof(u32);
    sqe.prd_table_length = 0;

    memcpy(tb_mem_ptr(&tb->ram, TB_SQ_BASE + tb->next_sq_tail), &sqe,
           sizeof(sqe));
    tb->next_sq_tail =
        (tb->next_sq_tail + UFSHC_SQ_ENTRY_SIZE) %
        (TB_QUEUE_DEPTH * UFSHC_SQ_ENTRY_SIZE);
    tb_apb_write(tb, UFSHC_MCQ_OP_BASE + UFSHC_OP_A_SQTP, tb->next_sq_tail);
    RUN_POLL_UNTIL(tb_cond_irq, 50000);
    return tb->next_cq_head;
}

static u32 tb_submit_cmd_prdt(ufshc_tb_t *tb, u32 cmd_addr, u32 data_addr,
                              u32 data_len)
{
    ufshc_utrd_t sqe;
    ufshc_prdt_t prdt;
    memset(&sqe, 0, sizeof(sqe));
    memset(&prdt, 0, sizeof(prdt));

    prdt.addr = data_addr;
    prdt.size = data_len - 1u;
    memcpy(tb_mem_ptr(&tb->ram, cmd_addr + TB_PRDT_OFF), &prdt, sizeof(prdt));

    sqe.dword_0 = 1u << 28;
    sqe.dword_2 = UFSHC_OCS_INVALID_COMMAND;
    sqe.command_desc_base_addr_lo = cmd_addr;
    sqe.response_upiu_offset = TB_RSP_OFF / sizeof(u32);
    sqe.response_upiu_length = sizeof(ufshc_upiu_rsp_t) / sizeof(u32);
    sqe.prd_table_offset = TB_PRDT_OFF / sizeof(u32);
    sqe.prd_table_length = 1;

    memcpy(tb_mem_ptr(&tb->ram, TB_SQ_BASE + tb->next_sq_tail), &sqe,
           sizeof(sqe));
    tb->next_sq_tail =
        (tb->next_sq_tail + UFSHC_SQ_ENTRY_SIZE) %
        (TB_QUEUE_DEPTH * UFSHC_SQ_ENTRY_SIZE);
    tb_apb_write(tb, UFSHC_MCQ_OP_BASE + UFSHC_OP_A_SQTP, tb->next_sq_tail);
    RUN_POLL_UNTIL(tb_cond_irq, 50000);
    return tb->next_cq_head;
}

static ufshc_cqe_t *tb_cqe(ufshc_tb_t *tb, u32 cq_off)
{
    return (ufshc_cqe_t *)tb_mem_ptr(&tb->ram, TB_CQ_BASE + cq_off);
}

static ufshc_upiu_rsp_t *tb_rsp(ufshc_tb_t *tb, u32 cmd_addr)
{
    return (ufshc_upiu_rsp_t *)tb_mem_ptr(&tb->ram, cmd_addr + TB_RSP_OFF);
}

static void tb_consume_cq(ufshc_tb_t *tb)
{
    tb->next_cq_head =
        (tb->next_cq_head + UFSHC_CQ_ENTRY_SIZE) %
        (TB_QUEUE_DEPTH * UFSHC_CQ_ENTRY_SIZE);
    tb_apb_write(tb, UFSHC_MCQ_OP_BASE + UFSHC_OP_A_CQHP, tb->next_cq_head);
    tb_apb_write(tb, UFSHC_MCQ_OP_BASE + UFSHC_OP_A_CQIS, UFSHC_CQIS_TEPS);
    tb_apb_write(tb, UFSHC_A_IS, UFSHC_IS_CQES);
}

static bool tb_read_backing_payload(ufshc_tb_t *tb, u32 off, u8 *buf, u32 len)
{
    FILE *fp = fopen(tb->image_path, "rb");
    if (fp == NULL) {
        return false;
    }
    ufshc_image_header_t hdr;
    bool ok = fread(&hdr, 1, sizeof(hdr), fp) == sizeof(hdr) &&
        memcmp(hdr.magic, UFSHC_IMAGE_MAGIC, UFSHC_IMAGE_MAGIC_SIZE) == 0 &&
        fseek(fp, (long)(UFSHC_IMAGE_HEADER_SIZE + off), SEEK_SET) == 0 &&
        fread(buf, 1, len, fp) == len;
    fclose(fp);
    return ok;
}

TEST_CASE(ufshc_tb_t, caps_and_link)
{
    TEST_BEGIN("MCQ capability and link startup");

    u32 cap = tb_apb_read(tb, UFSHC_A_CAP);
    u32 mcqcap = tb_apb_read(tb, UFSHC_A_MCQCAP);
    REQUIRE((cap & UFSHC_CAP_MCQS) != 0, "CAP.MCQS is set");
    REQUIRE((mcqcap & 0xffu) == UFSHC_MAX_QNUM - 1u, "MCQCAP.MAXQ");

    tb_apb_write(tb, UFSHC_A_HCE, UFSHC_HCE_HCE);
    REQUIRE((tb_apb_read(tb, UFSHC_A_HCS) & UFSHC_HCS_UCRDY) != 0,
            "HCS.UCRDY after HCE");
    tb_apb_write(tb, UFSHC_A_UICCMD, UFSHC_UIC_CMD_DME_LINK_STARTUP);
    REQUIRE((tb_apb_read(tb, UFSHC_A_HCS) & UFSHC_HCS_DP) != 0,
            "HCS.DP after link startup");

    TEST_END();
}

TEST_CASE(ufshc_tb_t, mcq_nop)
{
    TEST_BEGIN("MCQ NOP UPIU");
    tb_config_mcq(tb);

    u32 cmd = TB_CMD_BASE;
    ufshc_upiu_req_t req;
    memset(&req, 0, sizeof(req));
    req.header.trans_type = UFSHC_UPIU_NOP_OUT;
    req.header.task_tag = 1;
    memcpy(tb_mem_ptr(&tb->ram, cmd), &req, sizeof(req));

    u32 cq_off = tb_submit_cmd(tb, cmd);
    ufshc_cqe_t *cqe = tb_cqe(tb, cq_off);
    ufshc_upiu_rsp_t *rsp = tb_rsp(tb, cmd);
    REQUIRE(cqe->status == UFSHC_OCS_SUCCESS, "CQE status success");
    REQUIRE(rsp->header.trans_type == UFSHC_UPIU_NOP_IN, "NOP response");
    tb_consume_cq(tb);

    TEST_END();
}

TEST_CASE(ufshc_tb_t, query_device_desc)
{
    TEST_BEGIN("Query device descriptor");
    tb_config_mcq(tb);

    u32 cmd = TB_CMD_BASE + TB_CMD_STRIDE;
    ufshc_upiu_req_t req;
    memset(&req, 0, sizeof(req));
    req.header.trans_type = UFSHC_UPIU_QUERY_REQ;
    req.header.query_func = UFSHC_UPIU_QUERY_READ;
    req.header.task_tag = 2;
    req.qr.opcode = UFSHC_QUERY_OPCODE_READ_DESC;
    req.qr.idn = UFSHC_DESC_IDN_DEVICE;
    req.qr.length = tb_be16(89);
    memcpy(tb_mem_ptr(&tb->ram, cmd), &req, sizeof(req));

    u32 cq_off = tb_submit_cmd(tb, cmd);
    ufshc_cqe_t *cqe = tb_cqe(tb, cq_off);
    ufshc_upiu_rsp_t *rsp = tb_rsp(tb, cmd);
    REQUIRE(cqe->status == UFSHC_OCS_SUCCESS, "CQE status success");
    REQUIRE(rsp->header.trans_type == UFSHC_UPIU_QUERY_RSP, "query rsp");
    REQUIRE(rsp->header.response == UFSHC_QUERY_RESULT_SUCCESS, "query ok");
    REQUIRE(rsp->qr.data[0] == 89, "device descriptor length");
    REQUIRE(rsp->qr.data[6] == 1, "one logical unit");
    tb_consume_cq(tb);

    TEST_END();
}

TEST_CASE(ufshc_tb_t, scsi_write_read)
{
    TEST_BEGIN("SCSI WRITE(10) then READ(10)");
    tb_config_mcq(tb);

    const u32 len = UFSHC_BLOCK_SIZE;
    u32 write_cmd = TB_CMD_BASE + 2 * TB_CMD_STRIDE;
    u32 read_cmd = TB_CMD_BASE + 3 * TB_CMD_STRIDE;
    u32 write_buf = TB_DATA_BASE;
    u32 read_buf = TB_DATA_BASE + 0x4000u;

    for (u32 i = 0; i < len; i++) {
        tb_mem_ptr(&tb->ram, write_buf)[i] = (u8)(i ^ 0xa5u);
        tb_mem_ptr(&tb->ram, read_buf)[i] = 0;
    }

    ufshc_upiu_req_t req;
    memset(&req, 0, sizeof(req));
    req.header.trans_type = UFSHC_UPIU_COMMAND;
    req.header.flags = UFSHC_UPIU_FLAG_WRITE;
    req.header.task_tag = 3;
    req.sc.exp_data_transfer_len = tb_be32(len);
    req.sc.cdb[0] = UFSHC_SCSI_WRITE_10;
    req.sc.cdb[5] = 2;
    req.sc.cdb[8] = 1;
    memcpy(tb_mem_ptr(&tb->ram, write_cmd), &req, sizeof(req));

    u32 cq_off = tb_submit_cmd_prdt(tb, write_cmd, write_buf, len);
    REQUIRE(tb_cqe(tb, cq_off)->status == UFSHC_OCS_SUCCESS,
            "WRITE(10) status success");
    u8 file_buf[UFSHC_BLOCK_SIZE];
    REQUIRE(tb_read_backing_payload(tb, 2u * UFSHC_BLOCK_SIZE, file_buf, len),
            "WRITE(10) data is readable from backing file");
    REQUIRE(memcmp(tb_mem_ptr(&tb->ram, write_buf), file_buf, len) == 0,
            "backing file payload matches WRITE data");
    tb_consume_cq(tb);

    memset(&req, 0, sizeof(req));
    req.header.trans_type = UFSHC_UPIU_COMMAND;
    req.header.flags = UFSHC_UPIU_FLAG_READ;
    req.header.task_tag = 4;
    req.sc.exp_data_transfer_len = tb_be32(len);
    req.sc.cdb[0] = UFSHC_SCSI_READ_10;
    req.sc.cdb[5] = 2;
    req.sc.cdb[8] = 1;
    memcpy(tb_mem_ptr(&tb->ram, read_cmd), &req, sizeof(req));

    cq_off = tb_submit_cmd_prdt(tb, read_cmd, read_buf, len);
    REQUIRE(tb_cqe(tb, cq_off)->status == UFSHC_OCS_SUCCESS,
            "READ(10) status success");
    REQUIRE(memcmp(tb_mem_ptr(&tb->ram, write_buf),
                   tb_mem_ptr(&tb->ram, read_buf), len) == 0,
            "READ data matches previous WRITE data");
    tb_consume_cq(tb);

    TEST_END();
}

int main(void)
{
    ufshc_tb_t tb;
    tb_construct(&tb, "tb");

    tb_reset(&tb);
    TEST_RUN(caps_and_link);

    tb_reset(&tb);
    TEST_RUN(mcq_nop);

    tb_reset(&tb);
    TEST_RUN(query_device_desc);

    tb_reset(&tb);
    TEST_RUN(scsi_write_read);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
