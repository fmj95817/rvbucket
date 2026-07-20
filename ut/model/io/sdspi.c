#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "io/sdspi.h"
#include "mem/ram.h"
#include "sdspi_vip.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_SDSPI_BASE 0x30100000u
#define TB_RAM_BASE   0x40000000u
#define TB_RAM_SIZE   0x20000u
#define TB_IMAGE_SIZE (8u * 1024u * 1024u)
#define TB_TIMEOUT    10000u
#define TB_SD_OCR     0xc0ff8000u

typedef struct sdspi_tb {
    mod_t mod;
    u64 cycle_val;

    APB_IF_DECL(apb_);
    AXI4_IF_DECL(dma_);
    itf_t cmd_itf;
    itf_t data_itf;
    itf_t irq_itf;
    ext_irq_if_t *irq_o;

    sdspi_t dut;
    sdspi_vip_t vip;
    ram_t ram;
    char image_path[64];
    bool dut_clock_enable;
    bool vip_clock_enable;
    bool saw_write_overlap;
    bool saw_read_overlap;
    bool saw_dma_readback;

    ut_sbd_t sbd;
} sdspi_tb_t;

static void tb_make_image(sdspi_tb_t *tb)
{
    strcpy(tb->image_path, "/tmp/rvbucket-sdspi-ut-XXXXXX");
    int fd = mkstemp(tb->image_path);
    DBG_CHECK(fd >= 0);
    DBG_CHECK(ftruncate(fd, TB_IMAGE_SIZE) == 0);

    u8 data[2 * SDSPI_SECTOR_SIZE];
    for (u32 i = 0; i < sizeof(data); i++) {
        data[i] = (u8)(i ^ 0x5au);
    }
    DBG_CHECK(pwrite(fd, data, sizeof(data), 4 * SDSPI_SECTOR_SIZE) ==
        (ssize_t)sizeof(data));
    DBG_CHECK(close(fd) == 0);
}

static void tb_construct(sdspi_tb_t *tb, const char *name)
{
    memset(tb, 0, sizeof(*tb));
    tb_make_image(tb);
    tb->mod.cycle = &tb->cycle_val;
    mod_construct(&tb->mod, NULL, name);
    DBG_VCD_MODULE_SCOPE(name);
    dbg_vcd_set_clk(tb->mod.cycle);

    APB_IF_CONSTRUCT(tb, apb_, 2);
    AXI4_IF_CONSTRUCT(tb, dma_, 4);
    SDSPI_CMD_IF_CONSTRUCT(tb, cmd_itf, 2);
    SDSPI_DATA_IF_CONSTRUCT(tb, data_itf, 2);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, irq_itf, true, false);
    tb->irq_o = itf_signal_get_src_and_chk(&tb->irq_itf);

    APB_SLV_CONNECT(&tb->dut, , tb, apb_);
    AXI4_MST_CONNECT(&tb->dut, dma_, tb, dma_);
    tb->dut.irq_out = &tb->irq_itf;
    tb->dut.cmd_mst = &tb->cmd_itf;
    tb->dut.data_slv = &tb->data_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    sdspi_conf_t conf = {
        .base_addr = TB_SDSPI_BASE,
        .size = 0x1000
    };
    sdspi_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    tb->vip.mod.cycle = tb->mod.cycle;
    tb->vip.cmd_slv = &tb->cmd_itf;
    tb->vip.data_mst = &tb->data_itf;
    sdspi_vip_conf_t vip_conf = { .image_path = tb->image_path };
    sdspi_vip_construct(&tb->vip, tb->mod.hier_name, "u_vip", &vip_conf);

    AXI4_SLV_CONNECT(&tb->ram, , tb, dma_);
    tb->ram.mod.cycle = tb->mod.cycle;
    ram_construct(&tb->ram, tb->mod.hier_name, "u_ram", 1, RAM_MODE_AXI,
        TB_RAM_SIZE, TB_RAM_BASE, 1);
    ut_sbd_init(&tb->sbd);
}

static void tb_reset(sdspi_tb_t *tb)
{
    tb->dut_clock_enable = true;
    tb->vip_clock_enable = true;
    tb->saw_write_overlap = false;
    tb->saw_read_overlap = false;
    tb->saw_dma_readback = false;
    APB_IF_RESET(tb, apb_);
    AXI4_IF_RESET(tb, dma_);
    itf_reset(&tb->cmd_itf);
    itf_reset(&tb->data_itf);
    itf_reset(&tb->irq_itf);
    ram_reset(&tb->ram);
    sdspi_reset(&tb->dut);
    sdspi_vip_reset(&tb->vip);
    dbg_vcd_reset();
}

static void tb_clock(sdspi_tb_t *tb)
{
    if (tb->dut_clock_enable) {
        sdspi_clock(&tb->dut);
    }
    if (tb->vip_clock_enable) {
        sdspi_vip_clock(&tb->vip);
    }
    ram_clock(&tb->ram);
    if (tb->dut.state == SDSPI_STATE_WRITE_STREAM &&
        tb->dut.protocol_offset != 0 &&
        tb->dut.dma_offset < tb->dut.data_len) {
        tb->saw_write_overlap = true;
    }
    if (tb->dut.state == SDSPI_STATE_READ_STREAM &&
        tb->dut.dma_offset != 0 &&
        tb->dut.protocol_offset < tb->dut.data_len) {
        tb->saw_read_overlap = true;
    }
    if (tb->dut.dma_state == SDSPI_DMA_TO_MEM_SYNC_AR ||
        tb->dut.dma_state == SDSPI_DMA_TO_MEM_SYNC_R) {
        tb->saw_dma_readback = true;
    }
    APB_IF_DBG_CLOCK(tb, apb_);
    AXI4_IF_DBG_CLOCK(tb, dma_);
    itf_dbg_clock(&tb->cmd_itf);
    itf_dbg_clock(&tb->data_itf);
    itf_dbg_clock(&tb->irq_itf);
    tb->cycle_val++;
    dbg_vcd_clock();
}

static void tb_free(sdspi_tb_t *tb)
{
    sdspi_free(&tb->dut);
    sdspi_vip_free(&tb->vip);
    ram_free(&tb->ram);
    APB_IF_FREE(tb, apb_);
    AXI4_IF_FREE(tb, dma_);
    itf_free(&tb->cmd_itf);
    itf_free(&tb->data_itf);
    itf_free(&tb->irq_itf);
    mod_free(&tb->mod);
    unlink(tb->image_path);
}

static bool tb_apb_rsp_ready(sdspi_tb_t *tb)
{
    return !itf_fifo_empty(&tb->apb_apb_rsp_itf);
}

static bool tb_apb_write_error(sdspi_tb_t *tb, u32 offset, u32 value)
{
    apb_req_if_t req = {
        .paddr = TB_SDSPI_BASE + offset,
        .pwrite = true,
        .pwdata = value,
        .pstrb = 0x0f
    };
    itf_write(&tb->apb_apb_req_itf, &req);
    DBG_CHECK(RUN_POLL_UNTIL(tb_apb_rsp_ready, UT_TIMEOUT));
    apb_rsp_if_t rsp;
    itf_read(&tb->apb_apb_rsp_itf, &rsp);
    return rsp.pslverr;
}

static void tb_apb_write(sdspi_tb_t *tb, u32 offset, u32 value)
{
    DBG_CHECK(!tb_apb_write_error(tb, offset, value));
}

static u32 tb_apb_read(sdspi_tb_t *tb, u32 offset)
{
    apb_req_if_t req = {
        .paddr = TB_SDSPI_BASE + offset,
        .pwrite = false,
        .pwdata = 0,
        .pstrb = 0x0f
    };
    itf_write(&tb->apb_apb_req_itf, &req);
    DBG_CHECK(RUN_POLL_UNTIL(tb_apb_rsp_ready, UT_TIMEOUT));
    apb_rsp_if_t rsp;
    itf_read(&tb->apb_apb_rsp_itf, &rsp);
    DBG_CHECK(!rsp.pslverr);
    return rsp.prdata;
}

static bool tb_cmd_done(sdspi_tb_t *tb)
{
    return (tb->dut.cmd_status & SDSPI_CMD_STATUS_DONE) != 0;
}

static bool tb_write_streaming(sdspi_tb_t *tb)
{
    return tb->dut.state == SDSPI_STATE_WRITE_STREAM;
}

static bool tb_read_streaming(sdspi_tb_t *tb)
{
    return tb->vip.state == SDSPI_VIP_SEND_READ_DATA;
}

static void tb_clear_irq(sdspi_tb_t *tb)
{
    tb_apb_write(tb, SDSPI_REG_IRQ_STATUS, SDSPI_IRQ_MASK);
}

static void tb_start_cmd(sdspi_tb_t *tb, u32 opcode, u32 arg,
    u32 dma_addr, u32 block_size, u32 block_count, bool data, bool write)
{
    tb_clear_irq(tb);
    tb_apb_write(tb, SDSPI_REG_CMD_ARG, arg);
    if (data) {
        tb_apb_write(tb, SDSPI_REG_DMA_ADDR, dma_addr);
        tb_apb_write(tb, SDSPI_REG_BLOCK_SIZE, block_size);
        tb_apb_write(tb, SDSPI_REG_BLOCK_COUNT, block_count);
    }
    u32 ctrl = opcode | SDSPI_CMD_CTRL_START | SDSPI_CMD_CTRL_RSP_MASK;
    if (data) {
        ctrl |= SDSPI_CMD_CTRL_DATA;
    }
    if (write) {
        ctrl |= SDSPI_CMD_CTRL_WRITE;
    }
    tb_apb_write(tb, SDSPI_REG_CMD_CTRL, ctrl);
    DBG_CHECK(RUN_POLL_UNTIL(tb_cmd_done, TB_TIMEOUT));
}

static void tb_init_card(sdspi_tb_t *tb)
{
    tb_apb_write(tb, SDSPI_REG_CLOCK, 250);
    tb_apb_write(tb, SDSPI_REG_CTRL, SDSPI_CTRL_ENABLE);
    tb_apb_write(tb, SDSPI_REG_IRQ_ENABLE, SDSPI_IRQ_MASK);
    tb_start_cmd(tb, 0, 0, 0, 0, 0, false, false);
    tb_start_cmd(tb, 8, 0x1aau, 0, 0, 0, false, false);
    tb_start_cmd(tb, 55, 0, 0, 0, 0, false, false);
    tb_start_cmd(tb, 41, 0x40000000u, 0, 0, 0, false, false);
    tb_start_cmd(tb, 58, 0, 0, 0, 0, false, false);
}

TEST_CASE(sdspi_tb_t, discovery_and_init)
{
    TEST_BEGIN("Discovery and SPI initialization");

    REQUIRE(tb_apb_read(tb, SDSPI_REG_VERSION) == SDSPI_VERSION,
        "version register");
    u32 cap = tb_apb_read(tb, SDSPI_REG_CAP);
    REQUIRE((cap & (SDSPI_CAP_SPI_MODE | SDSPI_CAP_DMA)) ==
        (SDSPI_CAP_SPI_MODE | SDSPI_CAP_DMA), "SPI and DMA capabilities");
    REQUIRE((tb_apb_read(tb, SDSPI_REG_STATUS) &
        SDSPI_STATUS_CARD_PRESENT) != 0, "card is present");

    tb_init_card(tb);
    REQUIRE(tb->vip.enabled, "enable configuration reaches protocol peer");
    REQUIRE(tb->vip.clock_div == 250,
        "initialization clock divider reaches protocol peer");
    REQUIRE((tb_apb_read(tb, SDSPI_REG_STATUS) &
        SDSPI_STATUS_CARD_IDLE) == 0, "ACMD41 leaves idle state");
    REQUIRE(tb_apb_read(tb, SDSPI_REG_RESP1) == TB_SD_OCR,
        "CMD58 returns SDHC OCR");
    REQUIRE(tb->irq_o->irq, "completion IRQ asserted");
    tb_clear_irq(tb);
    REQUIRE(!tb->irq_o->irq, "W1C deasserts IRQ");

    tb_apb_write(tb, SDSPI_REG_CLOCK, 4);
    tb_clock(tb);
    REQUIRE(tb->vip.clock_div == 4,
        "data clock divider reaches protocol peer");

    TEST_END();
}

TEST_CASE(sdspi_tb_t, register_data)
{
    TEST_BEGIN("CSD and SCR data transfers");
    tb_init_card(tb);

    u32 csd_addr = TB_RAM_BASE + 0x100;
    memset(tb->ram.data + 0x100, 0, 16);
    tb_start_cmd(tb, 9, 0, csd_addr, 16, 1, true, false);
    REQUIRE(tb->ram.data[0x100] == 0x40, "CSD v2 transferred by DMA");
    REQUIRE((tb_apb_read(tb, SDSPI_REG_DATA_STATUS) &
        SDSPI_DATA_STATUS_ERROR) == 0, "CSD DMA completed without error");

    u32 scr_addr = TB_RAM_BASE + 0x200;
    tb_start_cmd(tb, 55, 0, 0, 0, 0, false, false);
    tb_start_cmd(tb, 51, 0, scr_addr, 8, 1, true, false);
    REQUIRE(tb->ram.data[0x200] == 0x02 && tb->ram.data[0x201] == 0x25,
        "SCR v2 data transferred by DMA");

    TEST_END();
}

TEST_CASE(sdspi_tb_t, block_read_write)
{
    TEST_BEGIN("Single and multi-block DMA");
    tb_init_card(tb);

    u32 read_offset = 0xff0;
    memset(tb->ram.data + read_offset, 0, 2 * SDSPI_SECTOR_SIZE);
    tb_start_cmd(tb, 18, 4, TB_RAM_BASE + read_offset,
        SDSPI_SECTOR_SIZE, 2, true, false);
    bool read_ok = true;
    for (u32 i = 0; i < 2 * SDSPI_SECTOR_SIZE; i++) {
        if (tb->ram.data[read_offset + i] != (u8)(i ^ 0x5au)) {
            read_ok = false;
            break;
        }
    }
    REQUIRE(read_ok, "multi-block read crosses a 4 KiB AXI boundary");
    REQUIRE(tb->saw_read_overlap,
        "read data reaches AXI before the protocol transfer completes");
    REQUIRE(tb->saw_dma_readback,
        "read DMA completes only after the AXI visibility drain");
    REQUIRE(tb->dut.dma_sync_offset == 2 * SDSPI_SECTOR_SIZE,
        "visibility drain covers the complete DMA destination");

    u32 write_offset = 0x3000;
    for (u32 i = 0; i < SDSPI_SECTOR_SIZE; i++) {
        tb->ram.data[write_offset + i] = (u8)(0xa5u ^ i);
    }
    tb_start_cmd(tb, 24, 7, TB_RAM_BASE + write_offset,
        SDSPI_SECTOR_SIZE, 1, true, true);
    u8 persisted[SDSPI_SECTOR_SIZE];
    int image_fd = open(tb->image_path, O_RDONLY);
    DBG_CHECK(image_fd >= 0);
    DBG_CHECK(pread(image_fd, persisted, sizeof(persisted),
        7 * SDSPI_SECTOR_SIZE) == (ssize_t)sizeof(persisted));
    DBG_CHECK(close(image_fd) == 0);
    REQUIRE(memcmp(persisted, tb->ram.data + write_offset,
        sizeof(persisted)) == 0, "single-block write persists to raw image");
    REQUIRE(tb->saw_write_overlap,
        "write reaches protocol peer before AXI prefetch completes");

    TEST_END();
}

TEST_CASE(sdspi_tb_t, command_while_busy)
{
    TEST_BEGIN("Command rejection while DMA is active");
    tb_init_card(tb);

    u32 read_offset = 0x5000;
    memset(tb->ram.data + read_offset, 0, 2 * SDSPI_SECTOR_SIZE);
    tb_clear_irq(tb);
    tb_apb_write(tb, SDSPI_REG_CMD_ARG, 4);
    tb_apb_write(tb, SDSPI_REG_DMA_ADDR, TB_RAM_BASE + read_offset);
    tb_apb_write(tb, SDSPI_REG_BLOCK_SIZE, SDSPI_SECTOR_SIZE);
    tb_apb_write(tb, SDSPI_REG_BLOCK_COUNT, 2);
    tb_apb_write(tb, SDSPI_REG_CMD_CTRL, 18 | SDSPI_CMD_CTRL_START |
        SDSPI_CMD_CTRL_RSP_MASK | SDSPI_CMD_CTRL_DATA);

    REQUIRE(tb_apb_write_error(tb, SDSPI_REG_CMD_CTRL,
        13 | SDSPI_CMD_CTRL_START | SDSPI_CMD_CTRL_RSP_MASK),
        "second command receives APB slave error");
    REQUIRE(RUN_POLL_UNTIL(tb_cmd_done, TB_TIMEOUT),
        "original DMA command completes");

    bool read_ok = true;
    for (u32 i = 0; i < 2 * SDSPI_SECTOR_SIZE; i++) {
        if (tb->ram.data[read_offset + i] != (u8)(i ^ 0x5au)) {
            read_ok = false;
            break;
        }
    }
    REQUIRE(read_ok, "rejected command does not abort active DMA");

    TEST_END();
}

TEST_CASE(sdspi_tb_t, interface_backpressure)
{
    TEST_BEGIN("Command and data interface backpressure");
    tb_init_card(tb);

    u32 write_offset = 0x6000;
    for (u32 i = 0; i < SDSPI_SECTOR_SIZE; i++) {
        tb->ram.data[write_offset + i] = (u8)(0x3cu ^ i);
    }
    tb_clear_irq(tb);
    tb_apb_write(tb, SDSPI_REG_CMD_ARG, 8);
    tb_apb_write(tb, SDSPI_REG_DMA_ADDR, TB_RAM_BASE + write_offset);
    tb_apb_write(tb, SDSPI_REG_BLOCK_SIZE, SDSPI_SECTOR_SIZE);
    tb_apb_write(tb, SDSPI_REG_BLOCK_COUNT, 1);
    tb_apb_write(tb, SDSPI_REG_CMD_CTRL, 24 | SDSPI_CMD_CTRL_START |
        SDSPI_CMD_CTRL_RSP_MASK | SDSPI_CMD_CTRL_DATA |
        SDSPI_CMD_CTRL_WRITE);
    REQUIRE(RUN_POLL_UNTIL(tb_write_streaming, TB_TIMEOUT),
        "write reaches packet stream");

    tb->vip_clock_enable = false;
    for (u32 i = 0; i < 4; i++) {
        tb_clock(tb);
    }
    u32 stalled_write_offset = tb->dut.protocol_offset;
    for (u32 i = 0; i < 8; i++) {
        tb_clock(tb);
    }
    REQUIRE(itf_fifo_full(&tb->cmd_itf), "write packet FIFO fills");
    REQUIRE(tb->dut.protocol_offset == stalled_write_offset,
        "controller holds write offset while VIP is backpressuring");
    tb->vip_clock_enable = true;
    REQUIRE(RUN_POLL_UNTIL(tb_cmd_done, TB_TIMEOUT),
        "write resumes after backpressure");

    u32 read_offset = 0x7000;
    memset(tb->ram.data + read_offset, 0, SDSPI_SECTOR_SIZE);
    tb_clear_irq(tb);
    tb_apb_write(tb, SDSPI_REG_CMD_ARG, 4);
    tb_apb_write(tb, SDSPI_REG_DMA_ADDR, TB_RAM_BASE + read_offset);
    tb_apb_write(tb, SDSPI_REG_BLOCK_SIZE, SDSPI_SECTOR_SIZE);
    tb_apb_write(tb, SDSPI_REG_BLOCK_COUNT, 1);
    tb_apb_write(tb, SDSPI_REG_CMD_CTRL, 17 | SDSPI_CMD_CTRL_START |
        SDSPI_CMD_CTRL_RSP_MASK | SDSPI_CMD_CTRL_DATA);
    REQUIRE(RUN_POLL_UNTIL(tb_read_streaming, TB_TIMEOUT),
        "read reaches packet stream");

    tb->dut_clock_enable = false;
    for (u32 i = 0; i < 4; i++) {
        tb_clock(tb);
    }
    u32 stalled_read_offset = tb->vip.data_offset;
    for (u32 i = 0; i < 8; i++) {
        tb_clock(tb);
    }
    REQUIRE(itf_fifo_full(&tb->data_itf), "read packet FIFO fills");
    REQUIRE(tb->vip.data_offset == stalled_read_offset,
        "VIP holds read offset while controller is backpressuring");
    tb->dut_clock_enable = true;
    REQUIRE(RUN_POLL_UNTIL(tb_cmd_done, TB_TIMEOUT),
        "read resumes after backpressure");

    TEST_END();
}

TEST_CASE(sdspi_tb_t, negative_paths)
{
    TEST_BEGIN("Absent card and invalid requests");
    tb_apb_write(tb, SDSPI_REG_CTRL, SDSPI_CTRL_ENABLE);
    tb_apb_write(tb, SDSPI_REG_IRQ_ENABLE, SDSPI_IRQ_MASK);

    sdspi_vip_set_card_present(&tb->vip, false);
    tb_clock(tb);
    tb_clock(tb);
    REQUIRE((tb_apb_read(tb, SDSPI_REG_STATUS) &
        SDSPI_STATUS_CARD_PRESENT) == 0, "card removal reaches controller");
    tb_start_cmd(tb, 8, 0x1aau, 0, 0, 0, false, false);
    REQUIRE((tb_apb_read(tb, SDSPI_REG_CMD_STATUS) &
        SDSPI_CMD_STATUS_TIMEOUT) != 0, "absent card reports timeout");
    sdspi_vip_set_card_present(&tb->vip, true);
    tb_clock(tb);
    tb_clock(tb);

    tb_init_card(tb);
    tb_start_cmd(tb, 17, TB_IMAGE_SIZE / SDSPI_SECTOR_SIZE,
        TB_RAM_BASE, SDSPI_SECTOR_SIZE, 1, true, false);
    REQUIRE((tb_apb_read(tb, SDSPI_REG_CMD_STATUS) &
        SDSPI_CMD_STATUS_ADDRESS) != 0, "out-of-range block is rejected");
    REQUIRE((tb_apb_read(tb, SDSPI_REG_IRQ_STATUS) & SDSPI_IRQ_ERROR) != 0,
        "invalid request raises error IRQ");

    tb_start_cmd(tb, 63, 0, 0, 0, 0, false, false);
    REQUIRE((tb_apb_read(tb, SDSPI_REG_CMD_STATUS) &
        SDSPI_CMD_STATUS_ILLEGAL) != 0, "unsupported command is illegal");

    TEST_END();
}

int main(void)
{
    sdspi_tb_t tb;
    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(discovery_and_init);
    tb_reset(&tb);
    TEST_RUN(register_data);
    tb_reset(&tb);
    TEST_RUN(block_read_write);
    tb_reset(&tb);
    TEST_RUN(command_while_busy);
    tb_reset(&tb);
    TEST_RUN(interface_backpressure);
    tb_reset(&tb);
    TEST_RUN(negative_paths);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);
    return ut_sbd_ret(&tb.sbd);
}
