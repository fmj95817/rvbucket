#include <stdio.h>
#include <stdlib.h>
#include "peri/peri.h"
#include "spec/peri.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct peri_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t apb_req_itf;
    itf_t apb_rsp_itf;
    itf_t uart_tx_itf;
    itf_t uart_rx_itf;
    itf_t uart_irq_itf;
    itf_t gpio_sig_itf;
    itf_t gpio_irq_itf;
    itf_t gtimer_irq_itf;
    itf_t ufshc_irq_itf;
    AXI4_IF_DECL(ufshc_dma_);

    ext_irq_if_t *irq_o;
    gpio_if_t *gpio_o;

    peri_t dut;

    ut_sbd_t sbd;
} peri_tb_t;

static void tb_construct(peri_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    APB_REQ_IF_CONSTRUCT(tb, apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(tb, apb_rsp_itf, 1);
    UART_IF_CONSTRUCT(tb, uart_tx_itf, 1);
    UART_IF_CONSTRUCT(tb, uart_rx_itf, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, uart_irq_itf, true, false);
    GPIO_SIGNAL_IF_CONSTRUCT(tb, gpio_sig_itf, true, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, gpio_irq_itf, true, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, gtimer_irq_itf, true, false);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, ufshc_irq_itf, true, false);
    AXI4_IF_CONSTRUCT(tb, ufshc_dma_, 4);

    tb->irq_o = itf_signal_get_src_and_chk(&tb->uart_irq_itf);
    tb->gpio_o = itf_signal_get_src_and_chk(&tb->gpio_sig_itf);

    tb->dut.mod.cycle = tb->mod.cycle;
    tb->dut.apb_req_slv = &tb->apb_req_itf;
    tb->dut.apb_rsp_mst = &tb->apb_rsp_itf;
    tb->dut.uart_tx_mst = &tb->uart_tx_itf;
    tb->dut.uart_rx_slv = &tb->uart_rx_itf;
    tb->dut.uart_irq_out = &tb->uart_irq_itf;
    tb->dut.gpio_inout = &tb->gpio_sig_itf;
    tb->dut.gpio_irq_out = &tb->gpio_irq_itf;
    tb->dut.gtimer_irq_out = &tb->gtimer_irq_itf;
    tb->dut.ufshc_irq_out = &tb->ufshc_irq_itf;
    AXI4_MST_CONNECT(&tb->dut, ufshc_, tb, ufshc_dma_);
    tb->dut.mod.cycle = tb->mod.cycle;
    peri_conf_t conf = {
        .base = 0x30000000,
        .size = PERI_UFSHC_OFFSET + PERI_UFSHC_SIZE,
        .ufshc = {
            .en = false,
            .backing_path = NULL
        }
    };
    peri_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(peri_tb_t *tb)
{
    peri_reset(&tb->dut);
    itf_reset(&tb->ufshc_irq_itf);
    AXI4_IF_RESET(tb, ufshc_dma_);
    dbg_vcd_reset();
}

static void tb_free(peri_tb_t *tb)
{
    peri_free(&tb->dut);
    itf_free(&tb->apb_req_itf);
    itf_free(&tb->apb_rsp_itf);
    itf_free(&tb->uart_tx_itf);
    itf_free(&tb->uart_rx_itf);
    itf_free(&tb->uart_irq_itf);
    itf_free(&tb->gpio_sig_itf);
    itf_free(&tb->gpio_irq_itf);
    itf_free(&tb->gtimer_irq_itf);
    itf_free(&tb->ufshc_irq_itf);
    AXI4_IF_FREE(tb, ufshc_dma_);
}

static void tb_clock(peri_tb_t *tb)
{
    peri_clock(&tb->dut);
    itf_dbg_clock(&tb->apb_req_itf);
    itf_dbg_clock(&tb->apb_rsp_itf);
    itf_dbg_clock(&tb->uart_tx_itf);
    itf_dbg_clock(&tb->uart_rx_itf);
    itf_dbg_clock(&tb->uart_irq_itf);
    itf_dbg_clock(&tb->gpio_sig_itf);
    itf_dbg_clock(&tb->gpio_irq_itf);
    itf_dbg_clock(&tb->gtimer_irq_itf);
    itf_dbg_clock(&tb->ufshc_irq_itf);
    AXI4_IF_DBG_CLOCK(tb, ufshc_dma_);
    (*tb->cycle)++; dbg_vcd_clock();
}

static bool tb_cond_apb_rsp_ready(peri_tb_t *tb) { return !itf_fifo_empty(&tb->apb_rsp_itf); }

static void tb_apb_write(peri_tb_t *tb, u32 addr, u32 data)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = true, .pwdata = data, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

static void tb_apb_read(peri_tb_t *tb, u32 addr)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = false, .pwdata = 0, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

TEST_CASE(peri_tb_t, uart_tx_via_apb)
{
    TEST_BEGIN("UART TX via APB Demux");

    tb_apb_write(tb, 0x30000004, 0x00000041);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "UART write: no slave error");
    }
    {
        REQUIRE(!itf_fifo_empty(&tb->uart_tx_itf), "UART TX FIFO has data");
        uart_if_t tx;
        itf_read(&tb->uart_tx_itf, &tx);
        REQUIRE(tx.data == 0x41, "UART TX data = 0x41");
    }

    TEST_END();
}

TEST_CASE(peri_tb_t, uart_rx_via_apb)
{
    TEST_BEGIN("UART RX via APB Demux");

    uart_if_t rx = { .data = 0x00000042 };
    itf_write(&tb->uart_rx_itf, &rx);
    tb_clock(tb);

    tb_apb_read(tb, 0x30000008);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0x42, "UART RX read = 0x42");
    }

    TEST_END();
}

TEST_CASE(peri_tb_t, gpio_via_apb)
{
    TEST_BEGIN("GPIO via APB Demux");

    tb_apb_write(tb, 0x30001000, 0x00adbeef);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "GPIO write: no slave error");
    }
    REQUIRE(tb->gpio_o->val == 0x00adbeef, "GPIO signal = 0x00adbeef");

    tb_apb_read(tb, 0x30001000);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0x00adbeef, "GPIO read = 0x00adbeef");
    }

    TEST_END();
}

TEST_CASE(peri_tb_t, uart_irq)
{
    TEST_BEGIN("UART IRQ via Peri");

    uart_if_t rx = { .data = 0x55 };
    itf_write(&tb->uart_rx_itf, &rx);
    tb_clock(tb);

    REQUIRE(tb->irq_o->irq, "IRQ asserted after UART RX");

    tb_apb_read(tb, 0x30000008);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    tb_clock(tb);
    REQUIRE(!tb->irq_o->irq, "IRQ deasserted after UART RX read");

    TEST_END();
}

int main(void)
{
    peri_tb_t tb;
    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(uart_tx_via_apb);
    TEST_RUN(uart_rx_via_apb);
    TEST_RUN(gpio_via_apb);
    TEST_RUN(uart_irq);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);
    return ut_sbd_ret(&tb.sbd);
}
