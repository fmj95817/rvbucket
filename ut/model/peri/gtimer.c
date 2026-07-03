#include <stdio.h>
#include <stdlib.h>
#include "peri/gtimer.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct gtimer_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t apb_req_itf;
    itf_t apb_rsp_itf;
    itf_t irq_sig_itf;

    ext_irq_if_t *irq_o;

    gtimer_t dut;

    ut_sbd_t sbd;
} gtimer_tb_t;

static void tb_construct(gtimer_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    APB_REQ_IF_CONSTRUCT(tb, apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(tb, apb_rsp_itf, 1);
    EXT_IRQ_SIGNAL_IF_CONSTRUCT(tb, irq_sig_itf, true, false);

    tb->irq_o = itf_signal_get_src_and_chk(&tb->irq_sig_itf);

    tb->dut.apb_req_slv = &tb->apb_req_itf;
    tb->dut.apb_rsp_mst = &tb->apb_rsp_itf;
    tb->dut.irq_out = &tb->irq_sig_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    gtimer_construct(&tb->dut, tb->mod.hier_name, "u_dut", 0x30002000, 16);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(gtimer_tb_t *tb)
{
    gtimer_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_free(gtimer_tb_t *tb)
{
    gtimer_free(&tb->dut);
    itf_free(&tb->apb_req_itf);
    itf_free(&tb->apb_rsp_itf);
    itf_free(&tb->irq_sig_itf);
}

static void tb_clock(gtimer_tb_t *tb)
{
    gtimer_clock(&tb->dut);
    itf_dbg_clock(&tb->apb_req_itf);
    itf_dbg_clock(&tb->apb_rsp_itf);
    itf_dbg_clock(&tb->irq_sig_itf);
    (*tb->cycle)++;
    dbg_vcd_clock();
}

static bool tb_cond_apb_rsp_ready(gtimer_tb_t *tb)
{
    return !itf_fifo_empty(&tb->apb_rsp_itf);
}

static void tb_apb_write(gtimer_tb_t *tb, u32 addr, u32 data)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = true, .pwdata = data, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

static void tb_apb_read(gtimer_tb_t *tb, u32 addr)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = false, .pwdata = 0, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

TEST_CASE(gtimer_tb_t, countdown)
{
    TEST_BEGIN("Countdown");

    tb_apb_write(tb, 0x30002008, 100);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "reload write: no slave error");
    }

    tb_apb_read(tb, 0x30002004);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 100, "count == 100 after reload");
    }

    tb_apb_write(tb, 0x30002000, 1);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    for (int i = 0; i < 100; i++) {
        tb_clock(tb);
    }

    tb_apb_read(tb, 0x30002004);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0, "count == 0 after 100 clocks");
    }

    TEST_END();
}

TEST_CASE(gtimer_tb_t, irq_on_expiry)
{
    TEST_BEGIN("IRQ on Expiry");

    tb_apb_write(tb, 0x30002008, 5);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    tb_apb_write(tb, 0x30002000, 1);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    for (int i = 0; i < 4; i++) {
        REQUIRE(!tb->irq_o->irq, "no IRQ before expiry");
        tb_clock(tb);
    }
    tb_clock(tb);
    REQUIRE(tb->irq_o->irq, "IRQ asserted on expiry");

    tb_apb_write(tb, 0x3000200C, 1);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }
    REQUIRE(!tb->irq_o->irq, "IRQ cleared after write");

    TEST_END();
}

TEST_CASE(gtimer_tb_t, no_count_when_disabled)
{
    TEST_BEGIN("No Count When Disabled");

    tb_apb_write(tb, 0x30002008, 10);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "reload write: no slave error");
    }

    for (int i = 0; i < 3; i++) {
        tb_clock(tb);
    }

    tb_apb_read(tb, 0x30002008);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 10, "reload unchanged when disabled");
    }

    TEST_END();
}

int main(void)
{
    gtimer_tb_t tb;
    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(countdown);
    TEST_RUN(irq_on_expiry);
    TEST_RUN(no_count_when_disabled);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);
    return ut_sbd_ret(&tb.sbd);
}
