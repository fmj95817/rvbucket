#include <stdio.h>
#include <string.h>

#include "io/io.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "spec/core/core.h"
#include "spec/io/sdspi.h"
#include "utils.h"

#define TB_DMA_ADDR 0x40002000u

typedef struct io_tb {
    mod_t mod;
    u64 cycle_val;

    APB_IF_DECL(apb_);
    AXI4_IF_DECL(dma_);
    itf_t sdspi_irq_itf;
    itf_t sdspi_cmd_itf;
    itf_t sdspi_data_itf;

    io_t dut;
    ut_sbd_t sbd;
} io_tb_t;

static void tb_construct(io_tb_t *tb, const char *name)
{
    memset(tb, 0, sizeof(*tb));
    tb->mod.cycle = &tb->cycle_val;
    mod_construct(&tb->mod, NULL, name);
    DBG_VCD_MODULE_SCOPE(name);
    dbg_vcd_set_clk(tb->mod.cycle);

    APB_IF_CONSTRUCT(tb, apb_, 2);
    AXI4_IF_CONSTRUCT(tb, dma_, 4);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, sdspi_irq_itf, true, false);
    SDSPI_CMD_IF_CONSTRUCT(tb, sdspi_cmd_itf, 8);
    SDSPI_DATA_IF_CONSTRUCT(tb, sdspi_data_itf, 2);

    tb->dut.mod.cycle = tb->mod.cycle;
    APB_SLV_CONNECT(&tb->dut, , tb, apb_);
    AXI4_MST_CONNECT(&tb->dut, dma_, tb, dma_);
    tb->dut.sdspi_irq_out = &tb->sdspi_irq_itf;
    tb->dut.sdspi_cmd_mst = &tb->sdspi_cmd_itf;
    tb->dut.sdspi_data_slv = &tb->sdspi_data_itf;
    io_conf_t conf = {
        .base = IO_SUBSYS_BASE,
        .size = IO_SUBSYS_SIZE
    };
    io_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);
    ut_sbd_init(&tb->sbd);
}

static void tb_reset(io_tb_t *tb)
{
    APB_IF_RESET(tb, apb_);
    AXI4_IF_RESET(tb, dma_);
    itf_reset(&tb->sdspi_irq_itf);
    itf_reset(&tb->sdspi_cmd_itf);
    itf_reset(&tb->sdspi_data_itf);
    io_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_clock(io_tb_t *tb)
{
    io_clock(&tb->dut);
    APB_IF_DBG_CLOCK(tb, apb_);
    AXI4_IF_DBG_CLOCK(tb, dma_);
    itf_dbg_clock(&tb->sdspi_irq_itf);
    itf_dbg_clock(&tb->sdspi_cmd_itf);
    itf_dbg_clock(&tb->sdspi_data_itf);
    tb->cycle_val++;
    dbg_vcd_clock();
}

static void tb_free(io_tb_t *tb)
{
    io_free(&tb->dut);
    APB_IF_FREE(tb, apb_);
    AXI4_IF_FREE(tb, dma_);
    itf_free(&tb->sdspi_irq_itf);
    itf_free(&tb->sdspi_cmd_itf);
    itf_free(&tb->sdspi_data_itf);
    mod_free(&tb->mod);
}

static bool tb_apb_rsp_ready(io_tb_t *tb)
{
    return !itf_fifo_empty(&tb->apb_apb_rsp_itf);
}

static u32 tb_apb_access(io_tb_t *tb, u32 offset, bool write, u32 data)
{
    apb_req_if_t req = {
        .paddr = IO_SUBSYS_BASE + offset,
        .pwrite = write,
        .pwdata = data,
        .pstrb = 0x0f
    };
    itf_write(&tb->apb_apb_req_itf, &req);
    DBG_CHECK(RUN_POLL_UNTIL(tb_apb_rsp_ready, UT_TIMEOUT));
    apb_rsp_if_t rsp;
    itf_read(&tb->apb_apb_rsp_itf, &rsp);
    DBG_CHECK(!rsp.pslverr);
    return rsp.prdata;
}

static void tb_apb_write(io_tb_t *tb, u32 offset, u32 data)
{
    (void)tb_apb_access(tb, offset, true, data);
}

static bool tb_dma_ar_ready(io_tb_t *tb)
{
    return !itf_fifo_empty(&tb->dma_axi4_ar_itf);
}

TEST_CASE(io_tb_t, apb_and_dma_routing)
{
    TEST_BEGIN("IO subsystem APB and DMA routing");

    REQUIRE(tb_apb_access(tb, SDSPI_REG_VERSION, false, 0) == SDSPI_VERSION,
        "IO APB window routes to SD-SPI");

    sdspi_data_if_t status = {
        .kind = SDSPI_DATA_KIND_STATUS,
        .card_present = true,
        .card_idle = true
    };
    itf_write(&tb->sdspi_data_itf, &status);
    tb_clock(tb);

    tb_apb_write(tb, SDSPI_REG_CLOCK, 250);
    tb_apb_write(tb, SDSPI_REG_CTRL, SDSPI_CTRL_ENABLE);
    tb_apb_write(tb, SDSPI_REG_DMA_ADDR, TB_DMA_ADDR);
    tb_apb_write(tb, SDSPI_REG_BLOCK_SIZE, SDSPI_SECTOR_SIZE);
    tb_apb_write(tb, SDSPI_REG_BLOCK_COUNT, 1);
    tb_apb_write(tb, SDSPI_REG_CMD_ARG, 4);
    tb_apb_write(tb, SDSPI_REG_CMD_CTRL,
        24 | (SDSPI_RSP_R1 << SDSPI_CMD_CTRL_RSP_SHIFT) |
        SDSPI_CMD_CTRL_DATA | SDSPI_CMD_CTRL_WRITE |
        SDSPI_CMD_CTRL_START);

    REQUIRE(RUN_POLL_UNTIL(tb_dma_ar_ready, UT_TIMEOUT),
        "SD-SPI DMA request reaches IO AXI master");
    axi4_ar_if_t ar;
    itf_read(&tb->dma_axi4_ar_itf, &ar);
    REQUIRE(ar.addr == TB_DMA_ADDR, "DMA address is preserved");
    REQUIRE(ar.len == 15 && ar.size == AXI4_AR_SIZE_B4,
        "IO AXI link preserves the prefetch burst");

    TEST_END();
}

int main(void)
{
    io_tb_t tb;
    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(apb_and_dma_routing);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);
    return ut_sbd_ret(&tb.sbd);
}
