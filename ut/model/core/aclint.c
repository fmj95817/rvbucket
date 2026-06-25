#include <stdio.h>
#include <stdlib.h>
#include "core/aclint.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

typedef struct aclint_tb {
    u64 *cycle;
    u64 cycle_val;

    itf_t apb_req_itf;
    itf_t apb_rsp_itf;
    itf_t core_timer_itf;
    itf_t core_m_irq_itf;
    itf_t core_s_irq_itf;
    itf_t core_swi_pend_itf;

    core_timer_if_t *core_timer_o;
    core_m_irq_if_t *core_m_irq_o;
    core_swi_pend_if_t *core_swi_pend_o;

    aclint_t dut;

    ut_sbd_t sbd;
} aclint_tb_t;

static void tb_construct(aclint_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;

    APB_REQ_IF_CONSTRUCT(tb, apb_req_itf, 1);
    APB_RSP_IF_CONSTRUCT(tb, apb_rsp_itf, 1);
    CORE_TIMER_SIGNAL_IF_CONSTRUCT(tb, core_timer_itf, true, false);
    CORE_M_IRQ_SIGNAL_IF_CONSTRUCT(tb, core_m_irq_itf, true, false);
    CORE_S_IRQ_IF_CONSTRUCT(tb, core_s_irq_itf, 1);
    CORE_SWI_PEND_SIGNAL_IF_CONSTRUCT(tb, core_swi_pend_itf, true, false);

    tb->core_timer_o = itf_signal_get_src_and_chk(&tb->core_timer_itf);
    tb->core_m_irq_o = itf_signal_get_src_and_chk(&tb->core_m_irq_itf);
    tb->core_swi_pend_o = itf_signal_get_src_and_chk(&tb->core_swi_pend_itf);

    aclint_conf_t conf = {
        .mtimer_base = 0x02000000,
        .mtimer_size = 0x1000,
        .mtimecmp_base = 0x02004000,
        .mtimecmp_size = 0x1000,
        .mswi_base = 0x02008000,
        .mswi_size = 0x1000,
        .sswi_base = 0x0200c000,
        .sswi_size = 0x1000,
        .mtimer_tick_cycles = 10
    };

    tb->dut.cycle = tb->cycle;
    tb->dut.cfg_apb_req_slv = &tb->apb_req_itf;
    tb->dut.cfg_apb_rsp_mst = &tb->apb_rsp_itf;
    tb->dut.core_timer_out = &tb->core_timer_itf;
    tb->dut.core_m_irq_outs[0] = &tb->core_m_irq_itf;
    tb->dut.core_s_irq_msts[0] = &tb->core_s_irq_itf;
    tb->dut.core_swi_pend_ins[0] = &tb->core_swi_pend_itf;
    aclint_construct(&tb->dut, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(aclint_tb_t *tb)
{
    aclint_reset(&tb->dut);
    dbg_vcd_reset();
}

static void tb_clock(aclint_tb_t *tb)
{
    aclint_clock(&tb->dut);

    itf_dbg_clock(&tb->apb_req_itf);
    itf_dbg_clock(&tb->apb_rsp_itf);
    itf_dbg_clock(&tb->core_timer_itf);
    itf_dbg_clock(&tb->core_m_irq_itf);
    itf_dbg_clock(&tb->core_s_irq_itf);
    itf_dbg_clock(&tb->core_swi_pend_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free(aclint_tb_t *tb)
{
    aclint_free(&tb->dut);
    itf_free(&tb->apb_req_itf);
    itf_free(&tb->apb_rsp_itf);
    itf_free(&tb->core_timer_itf);
    itf_free(&tb->core_m_irq_itf);
    itf_free(&tb->core_s_irq_itf);
    itf_free(&tb->core_swi_pend_itf);
}

static bool tb_cond_apb_rsp_ready(aclint_tb_t *tb)
{
    return !itf_fifo_empty(&tb->apb_rsp_itf);
}

static void tb_apb_write(aclint_tb_t *tb, u32 addr, u32 data, u8 strb)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = true, .pwdata = data, .pstrb = strb };
    itf_write(&tb->apb_req_itf, &r);
}

static void tb_apb_read(aclint_tb_t *tb, u32 addr)
{
    apb_req_if_t r = { .paddr = addr, .pwrite = false, .pwdata = 0, .pstrb = 0xf };
    itf_write(&tb->apb_req_itf, &r);
}

static u32 tb_apb_read_val(aclint_tb_t *tb, u32 addr)
{
    tb_apb_read(tb, addr);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    apb_rsp_if_t rsp;
    itf_read(&tb->apb_rsp_itf, &rsp);
    return rsp.prdata;
}

TEST_CASE(aclint_tb_t, read_mtime)
{
    TEST_BEGIN("Read mtime");
    u32 lo = tb_apb_read_val(tb, 0x02000000);
    REQUIRE(lo == 0, "mtime.lo = 0 after reset");

    RUN_CYCLES(10);

    lo = tb_apb_read_val(tb, 0x02000000);
    REQUIRE(lo == 1, "mtime.lo = 1 after 10 cycles");

    RUN_CYCLES(90);

    lo = tb_apb_read_val(tb, 0x02000000);
    REQUIRE(lo == 10, "mtime.lo = 10 after 100 cycles");
    TEST_END();
}

TEST_CASE(aclint_tb_t, write_mtimecmp)
{
    TEST_BEGIN("Write mtimecmp");
    tb_apb_write(tb, 0x02004000, 0x00000005, 0xf);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "mtimecmp.lo write: no slave error");
    }

    u32 lo = tb_apb_read_val(tb, 0x02004000);
    REQUIRE(lo == 5, "mtimecmp.lo = 5");
    TEST_END();
}

TEST_CASE(aclint_tb_t, timer_irq)
{
    TEST_BEGIN("Timer IRQ");
    tb_apb_write(tb, 0x02004004, 0, 0xf);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    tb_apb_write(tb, 0x02004000, 2, 0xf);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    RUN_CYCLES(20);
    REQUIRE(tb->core_m_irq_o->mtimer, "mtimer IRQ asserted (mtime >= mtimecmp)");

    tb_apb_write(tb, 0x02004004, 0x10000000, 0xf);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }

    tb_clock(tb);
    REQUIRE(!tb->core_m_irq_o->mtimer, "mtimer IRQ deasserted (mtime < mtimecmp)");
    TEST_END();
}

TEST_CASE(aclint_tb_t, mswi_write)
{
    TEST_BEGIN("MSWI Write");
    tb_apb_write(tb, 0x02008000, 1, 0x1);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "MSWI write: no slave error");
    }
    REQUIRE(tb->core_m_irq_o->msw, "msw IRQ asserted");

    tb_apb_write(tb, 0x02008000, 0, 0x1);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
    }
    REQUIRE(!tb->core_m_irq_o->msw, "msw IRQ deasserted");
    TEST_END();
}

TEST_CASE(aclint_tb_t, mswi_read)
{
    TEST_BEGIN("MSWI Read");
    tb->core_swi_pend_o->msip = true;
    u32 v = tb_apb_read_val(tb, 0x02008000);
    REQUIRE(v == 1, "MSWI read returns 1 (msip pending)");

    tb->core_swi_pend_o->msip = false;
    v = tb_apb_read_val(tb, 0x02008000);
    REQUIRE(v == 0, "MSWI read returns 0 (msip cleared)");
    TEST_END();
}

TEST_CASE(aclint_tb_t, sswi_write)
{
    TEST_BEGIN("SSWI Write");
    tb_apb_write(tb, 0x0200c000, 1, 0x1);
    RUN_POLL_UNTIL(tb_cond_apb_rsp_ready, UT_TIMEOUT);
    {
        apb_rsp_if_t rsp;
        itf_read(&tb->apb_rsp_itf, &rsp);
        REQUIRE(!rsp.pslverr, "SSWI write: no slave error");
    }

    REQUIRE(!itf_fifo_empty(&tb->core_s_irq_itf), "SSWI: core_s_irq FIFO has packet");
    {
        core_s_irq_if_t irq;
        itf_read(&tb->core_s_irq_itf, &irq);
        REQUIRE(irq.ssw, "SSWI: ssw flag set");
    }
    TEST_END();
}

int main()
{
    aclint_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(read_mtime);
    TEST_RUN(write_mtimecmp);
    TEST_RUN(timer_irq);
    TEST_RUN(mswi_write);
    TEST_RUN(mswi_read);
    TEST_RUN(sswi_write);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);

    return ret;
}
