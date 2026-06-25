#include <stdio.h>
#include <stdlib.h>
#include "io/uart.h"
#include "itf/ext_irq_if.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct uart_tb {
    u64 *cycle;
    u64 cycle_val;
    itf_t apb_req_itf, apb_rsp_itf, uart_tx_itf, uart_rx_itf, irq_out_itf;
    ext_irq_if_t *irq_o;
    uart_t dut;
    ut_sbd_t sbd;
} uart_tb_t;

static void tb_construct(uart_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    APB_REQ_IF_CONSTRUCT(tb, apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(tb, apb_rsp_itf, 1);
    UART_IF_CONSTRUCT(tb, uart_tx_itf, 1);
    UART_IF_CONSTRUCT(tb, uart_rx_itf, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, irq_out_itf, true, false);

    tb->irq_o = itf_signal_get_src_and_chk(&tb->irq_out_itf);
    tb->dut.apb_req_slv = &tb->apb_req_itf;
    tb->dut.apb_rsp_mst = &tb->apb_rsp_itf;
    tb->dut.uart_tx_mst = &tb->uart_tx_itf;
    tb->dut.uart_rx_slv = &tb->uart_rx_itf;
    tb->dut.irq_out = &tb->irq_out_itf;

    uart_construct(&tb->dut, "u_dut", 0x10000000, 0x1000);
    ut_sbd_init(&tb->sbd);
}

static void tb_reset(uart_tb_t *tb)
{
    uart_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_free(uart_tb_t *tb)
{
    uart_free(&tb->dut);
    itf_free(&tb->apb_req_itf);
    itf_free(&tb->apb_rsp_itf);
    itf_free(&tb->uart_tx_itf);
    itf_free(&tb->uart_rx_itf);
    itf_free(&tb->irq_out_itf);
}

static void tb_clock(uart_tb_t *tb)
{
    uart_clock(&tb->dut);
    itf_dbg_clock(&tb->apb_req_itf);
    itf_dbg_clock(&tb->apb_rsp_itf);
    itf_dbg_clock(&tb->uart_tx_itf);
    itf_dbg_clock(&tb->uart_rx_itf);
    itf_dbg_clock(&tb->irq_out_itf);
    (*tb->cycle)++;
    dbg_vcd_clock();
}

static bool tb_cond_apb_rsp_ready(uart_tb_t *tb)
{
    return !itf_fifo_empty(&tb->apb_rsp_itf);
}

static void tb_apb_write(uart_tb_t *tb, u32 addr, u32 data)
{
    apb_req_if_t r = {.paddr = addr, .pwrite = true, .pwdata = data, .pstrb = 0xf};
    itf_write(&tb->apb_req_itf, &r);
}

static void tb_apb_read(uart_tb_t *tb, u32 addr)
{
    apb_req_if_t r = {.paddr = addr, .pwrite = false, .pwdata = 0, .pstrb = 0xf};
    itf_write(&tb->apb_req_itf, &r);
}

TEST_CASE(uart_tb_t, tx_write)
{
    TEST_BEGIN("APB Write to TX");
 
    tb_apb_write(tb, 0x10000004, 0x00000041);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "APB response: no slave error");
    }
    {
        REQUIRE(!itf_fifo_empty(&tb->uart_tx_itf), "UART TX FIFO has data");
        uart_if_t tx;
        itf_read(&tb->uart_tx_itf, &tx);
        REQUIRE(tx.data == 0x41, "TX data = 0x41");
    }

    TEST_END();
}

TEST_CASE(uart_tb_t, rx_read)
{
    TEST_BEGIN("APB Read from RX");

    uart_if_t rx = {.data = 0x00000042};
    itf_write(&tb->uart_rx_itf, &rx);
    REQUIRE(!itf_fifo_empty(&tb->uart_rx_itf), "RX data injected");

    tb_clock(tb);
    REQUIRE(itf_fifo_empty(&tb->uart_rx_itf), "RX data consumed by UART");
    REQUIRE(tb->irq_o->irq, "IRQ asserted after RX");
    
    tb_apb_read(tb, 0x10000008);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0x42, "RX read returns 0x42");
        REQUIRE(!rsp.pslverr, "APB response: no slave error");
    }

    tb_clock(tb);
    REQUIRE(!tb->irq_o->irq, "IRQ deasserted after RX read");
    
    TEST_END();
}

TEST_CASE(uart_tb_t, status_read)
{
    TEST_BEGIN("Status Register");
 
    tb_apb_read(tb, 0x1000000c);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp0;
        itf_read(&tb->apb_rsp_itf, &rsp0);
        REQUIRE(rsp0.prdata == 0, "STS = 0 (no RX pending)");
    }
 
    uart_if_t rx = {.data = 0x55};
    itf_write(&tb->uart_rx_itf, &rx);
    tb_clock(tb);
 
    tb_apb_read(tb, 0x1000000c);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp1;
        itf_read(&tb->apb_rsp_itf, &rsp1);
        REQUIRE(rsp1.prdata == 1, "STS = 1 (RX valid)");
    }
    tb_apb_read(tb, 0x10000008);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0x55, "RX read = 0x55 (cleanup)");
    }

    TEST_END();
}

TEST_CASE(uart_tb_t, then_read)
{
    TEST_BEGIN("Write TX Then Read RX");

    tb_apb_write(tb, 0x10000004, 0x0000005a);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "TX write: APB rsp ok");
    }
    {
        REQUIRE(!itf_fifo_empty(&tb->uart_tx_itf), "TX write: UART TX has data");
        uart_if_t tx;
        itf_read(&tb->uart_tx_itf, &tx);
        REQUIRE(tx.data == 0x5a, "TX data = 0x5a");
    }
    
    uart_if_t rx = {.data = 0xa5};
    itf_write(&tb->uart_rx_itf, &rx);
    tb_clock(tb);
    
    tb_apb_read(tb, 0x10000008);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0xa5, "RX: read back 0xa5");
    }
    
    TEST_END();
}

int main()
{
    uart_tb_t tb;
    
    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(tx_write);
    TEST_RUN(rx_read);
    TEST_RUN(status_read);
    TEST_RUN(then_read);
    
    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);
    
    return ret;
}
