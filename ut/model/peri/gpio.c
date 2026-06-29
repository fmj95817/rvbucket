#include <stdio.h>
#include <stdlib.h>
#include "peri/gpio.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct gpio_tb {
    u64 *cycle;
    u64 cycle_val;

    itf_t apb_req_itf;
    itf_t apb_rsp_itf;
    itf_t out_sig_itf;
    gpio_if_t *out_o;

    gpio_t dut;

    ut_sbd_t sbd;
} gpio_tb_t;

static void tb_construct(gpio_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;

    APB_REQ_IF_CONSTRUCT(tb, apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(tb, apb_rsp_itf, 1);
    GPIO_SIGNAL_IF_CONSTRUCT(tb, out_sig_itf, true, false);

    tb->out_o = itf_signal_get_src_and_chk(&tb->out_sig_itf);

    tb->dut.apb_req_slv = &tb->apb_req_itf;
    tb->dut.apb_rsp_mst = &tb->apb_rsp_itf;
    tb->dut.inout_sig = &tb->out_sig_itf;
    gpio_construct(&tb->dut, "u_dut", 0x30001000, 16);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(gpio_tb_t *tb)
{
    gpio_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_free(gpio_tb_t *tb)
{
    gpio_free(&tb->dut);
    itf_free(&tb->apb_req_itf);
    itf_free(&tb->apb_rsp_itf);
    itf_free(&tb->out_sig_itf);
}

static void tb_clock(gpio_tb_t *tb)
{
    gpio_clock(&tb->dut);
    itf_dbg_clock(&tb->apb_req_itf);
    itf_dbg_clock(&tb->apb_rsp_itf);
    itf_dbg_clock(&tb->out_sig_itf);
    (*tb->cycle)++; dbg_vcd_clock();
}

static bool tb_cond_apb_rsp_ready(gpio_tb_t *tb) { return !itf_fifo_empty(&tb->apb_rsp_itf); }

static void tb_apb_write(gpio_tb_t *tb, u32 addr, u32 data)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = true, .pwdata = data, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

static void tb_apb_read(gpio_tb_t *tb, u32 addr)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = false, .pwdata = 0, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

TEST_CASE(gpio_tb_t, write_output)
{
    TEST_BEGIN("Write GPIO Output");

    tb_apb_write(tb, 0x30001000, 0xdeadbeef);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "APB write: no slave error");
    }
    REQUIRE(tb->out_o->val == 0xdeadbeef, "signal output = 0xdeadbeef");

    TEST_END();
}

TEST_CASE(gpio_tb_t, read_output)
{
    TEST_BEGIN("Read GPIO Output");

    tb_apb_write(tb, 0x30001000, 0xcafebabe);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    tb_apb_read(tb, 0x30001000);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(rsp.prdata == 0xcafebabe, "read back = 0xcafebabe");
    }

    TEST_END();
}

TEST_CASE(gpio_tb_t, write_then_overwrite)
{
    TEST_BEGIN("Write Then Overwrite");

    tb_apb_write(tb, 0x30001000, 0x11111111);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    tb_apb_write(tb, 0x30001000, 0x22222222);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    REQUIRE(tb->out_o->val == 0x22222222, "signal output = 0x22222222 (overwritten)");

    TEST_END();
}

int main(void)
{
    gpio_tb_t tb;
    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(write_output);
    TEST_RUN(read_output);
    TEST_RUN(write_then_overwrite);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);
    return ut_sbd_ret(&tb.sbd);
}
