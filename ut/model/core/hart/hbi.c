#include <stdio.h>
#include "core/hart/hbi.h"
#include "dbg/vcd.h"
#include "spec/core/hart.h"
#include "utils.h"

#define TB_HBI_STG_FIFO_DEPTH 8u
#define TB_HBI_I_OST_DEPTH 16u
#define TB_HBI_D_OST_DEPTH 16u

typedef struct hbi_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t i_bti_req_itf;
    itf_t i_bti_rsp_itf;
    itf_t d_bti_req_itf;
    itf_t d_bti_rsp_itf;
    itf_t fch_req_itf;
    itf_t fch_rsp_itf;
    itf_t mmu_fch_expt_itf;
    itf_t ldst_req_itf;
    itf_t ldst_rsp_itf;

    hbi_t dut;
    ut_sbd_t sbd;
} hbi_tb_t;

static void tb_construct(hbi_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, i_bti_req_itf, 4);
    BTI_RSP_IF_CONSTRUCT(tb, i_bti_rsp_itf, 4);
    BTI_REQ_IF_CONSTRUCT(tb, d_bti_req_itf, 4);
    BTI_RSP_IF_CONSTRUCT(tb, d_bti_rsp_itf, 4);
    FCH_REQ_IF_CONSTRUCT(tb, fch_req_itf, 4);
    FCH_RSP_IF_CONSTRUCT(tb, fch_rsp_itf, 4);
    HART_EXPT_IF_CONSTRUCT(tb, mmu_fch_expt_itf, 4);
    LDST_REQ_IF_CONSTRUCT(tb, ldst_req_itf, 4);
    LDST_RSP_IF_CONSTRUCT(tb, ldst_rsp_itf, 4);

    tb->dut.i_bti_req_mst = &tb->i_bti_req_itf;
    tb->dut.i_bti_rsp_slv = &tb->i_bti_rsp_itf;
    tb->dut.d_bti_req_mst = &tb->d_bti_req_itf;
    tb->dut.d_bti_rsp_slv = &tb->d_bti_rsp_itf;
    tb->dut.fch_req_slv = &tb->fch_req_itf;
    tb->dut.fch_rsp_mst = &tb->fch_rsp_itf;
    tb->dut.mmu_fch_expt_slv = &tb->mmu_fch_expt_itf;
    tb->dut.ldst_req_slv = &tb->ldst_req_itf;
    tb->dut.ldst_rsp_mst = &tb->ldst_rsp_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    hbi_conf_t conf = {
        .stg_fifo_depth = TB_HBI_STG_FIFO_DEPTH,
        .i_ost_depth = TB_HBI_I_OST_DEPTH,
        .d_ost_depth = TB_HBI_D_OST_DEPTH
    };
    hbi_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(hbi_tb_t *tb)
{
    hbi_reset(&tb->dut);
    itf_reset(&tb->i_bti_req_itf);
    itf_reset(&tb->i_bti_rsp_itf);
    itf_reset(&tb->d_bti_req_itf);
    itf_reset(&tb->d_bti_rsp_itf);
    itf_reset(&tb->fch_req_itf);
    itf_reset(&tb->fch_rsp_itf);
    itf_reset(&tb->mmu_fch_expt_itf);
    itf_reset(&tb->ldst_req_itf);
    itf_reset(&tb->ldst_rsp_itf);
    dbg_vcd_reset();
}

static void tb_free(hbi_tb_t *tb)
{
    hbi_free(&tb->dut);
    itf_free(&tb->i_bti_req_itf);
    itf_free(&tb->i_bti_rsp_itf);
    itf_free(&tb->d_bti_req_itf);
    itf_free(&tb->d_bti_rsp_itf);
    itf_free(&tb->fch_req_itf);
    itf_free(&tb->fch_rsp_itf);
    itf_free(&tb->mmu_fch_expt_itf);
    itf_free(&tb->ldst_req_itf);
    itf_free(&tb->ldst_rsp_itf);
}

static void tb_clock(hbi_tb_t *tb)
{
    hbi_clock(&tb->dut);

    itf_dbg_clock(&tb->i_bti_req_itf);
    itf_dbg_clock(&tb->i_bti_rsp_itf);
    itf_dbg_clock(&tb->d_bti_req_itf);
    itf_dbg_clock(&tb->d_bti_rsp_itf);
    itf_dbg_clock(&tb->fch_req_itf);
    itf_dbg_clock(&tb->fch_rsp_itf);
    itf_dbg_clock(&tb->mmu_fch_expt_itf);
    itf_dbg_clock(&tb->ldst_req_itf);
    itf_dbg_clock(&tb->ldst_rsp_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static bool tb_cond_i_bti_req_ready(hbi_tb_t *tb)
{
    return !itf_fifo_empty(&tb->i_bti_req_itf);
}

static bool tb_cond_d_bti_req_ready(hbi_tb_t *tb)
{
    return !itf_fifo_empty(&tb->d_bti_req_itf);
}

static bool tb_cond_fch_rsp_ready(hbi_tb_t *tb)
{
    return !itf_fifo_empty(&tb->fch_rsp_itf);
}

static bool tb_cond_ldst_rsp_ready(hbi_tb_t *tb)
{
    return !itf_fifo_empty(&tb->ldst_rsp_itf);
}

TEST_CASE(hbi_tb_t, fch_req_capture_one_per_cycle)
{
    TEST_BEGIN("FCH Request Capture One Per Cycle");

    tb_reset(tb);

    for (u32 i = 0; i < 3; i++) {
        fch_req_if_t req = { .pc = 0x80000000 + i * 4 };
        itf_write(&tb->fch_req_itf, &req);
    }

    tb_clock(tb);
    REQUIRE(tb->fch_req_itf.ctx.fifo.pkt_num == 2,
        "only one fch_req is captured in one clock");
    REQUIRE(tb->dut.fch_req_fifo.num == 0,
        "captured fch_req is issued downstream in same clock");

    TEST_END();
}

TEST_CASE(hbi_tb_t, i_rsp_order_expt_before_older_normal)
{
    TEST_BEGIN("I Response Order: Later Expt Before Older Normal");

    tb_reset(tb);

    for (u32 i = 0; i < 2; i++) {
        fch_req_if_t fch_req = { .pc = 0x80000000 + i * 4 };
        itf_write(&tb->fch_req_itf, &fch_req);

        RUN_POLL_UNTIL(tb_cond_i_bti_req_ready, UT_TIMEOUT);
        bti_req_if_t bti_req;
        itf_read(&tb->i_bti_req_itf, &bti_req);
        REQUIRE(bti_req.addr == fch_req.pc, "fch_req is issued to i_bti");
    }

    hart_expt_if_t expt = {
        .type = HART_EXPT_TYPE_EXCEPTION,
        .cause = HART_EXPT_CAUSE_INST_PAGE_FAULT,
        .priv = 1,
        .pc = 0x80000004,
        .tval = 0x80000004
    };
    itf_write(&tb->mmu_fch_expt_itf, &expt);
    tb_clock(tb);
    REQUIRE(itf_fifo_empty(&tb->fch_rsp_itf),
        "later expt waits for older normal response");

    bti_rsp_if_t bti_rsp = {
        .data = 0x00000013,
        .ok = true,
        .trans_id = FCH_TRANS_ID
    };
    itf_write(&tb->i_bti_rsp_itf, &bti_rsp);
    tb_clock(tb);

    RUN_POLL_UNTIL(tb_cond_fch_rsp_ready, UT_TIMEOUT);
    {
        fch_rsp_if_t fch_rsp;
        itf_read(&tb->fch_rsp_itf, &fch_rsp);
        REQUIRE(fch_rsp.ok && !fch_rsp.expt && fch_rsp.ir == 0x00000013,
            "older normal response is emitted first");
    }

    tb_clock(tb);
    RUN_POLL_UNTIL(tb_cond_fch_rsp_ready, UT_TIMEOUT);
    {
        fch_rsp_if_t fch_rsp;
        itf_read(&tb->fch_rsp_itf, &fch_rsp);
        REQUIRE(!fch_rsp.ok && fch_rsp.expt &&
            fch_rsp.cause == HART_EXPT_CAUSE_INST_PAGE_FAULT &&
            fch_rsp.tval == 0x80000004,
            "later exception response is emitted second");
    }

    TEST_END();
}

TEST_CASE(hbi_tb_t, d_req_multi_outstanding_order)
{
    TEST_BEGIN("D Request Multi Outstanding Order");

    tb_reset(tb);

    for (u32 i = 0; i < 2; i++) {
        ldst_req_if_t req = {
            .addr = 0x80001000 + i * 4,
            .st = false,
            .size = LDST_REQ_SIZE_B4,
            .data = 0,
            .strobe = 0xf
        };
        itf_write(&tb->ldst_req_itf, &req);
    }

    for (u32 i = 0; i < 2; i++) {
        RUN_POLL_UNTIL(tb_cond_d_bti_req_ready, UT_TIMEOUT);
        bti_req_if_t bti_req;
        itf_read(&tb->d_bti_req_itf, &bti_req);
        REQUIRE(bti_req.trans_id == LDST_TRANS_ID, "d request uses ldst trans id");
        REQUIRE(bti_req.addr == 0x80001000 + i * 4,
            "d request is issued in order");
    }
    REQUIRE(tb->dut.d_ost.count == 2, "two d requests are outstanding");

    for (u32 i = 0; i < 2; i++) {
        bti_rsp_if_t bti_rsp = {
            .trans_id = LDST_TRANS_ID,
            .data = 0x11110000 + i,
            .ok = true
        };
        itf_write(&tb->d_bti_rsp_itf, &bti_rsp);
    }

    for (u32 i = 0; i < 2; i++) {
        RUN_POLL_UNTIL(tb_cond_ldst_rsp_ready, UT_TIMEOUT);
        ldst_rsp_if_t ldst_rsp;
        itf_read(&tb->ldst_rsp_itf, &ldst_rsp);
        REQUIRE(ldst_rsp.ok, "ldst response is ok");
        REQUIRE(ldst_rsp.data == 0x11110000 + i,
            "ldst response is returned in issue order");
        tb_clock(tb);
    }

    TEST_END();
}

int main(void)
{
    hbi_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(fch_req_capture_one_per_cycle);
    TEST_RUN(i_rsp_order_expt_before_older_normal);
    TEST_RUN(d_req_multi_outstanding_order);

    ut_sbd_summary(&tb.sbd);
    tb_free(&tb);

    return ut_sbd_ret(&tb.sbd);
}
