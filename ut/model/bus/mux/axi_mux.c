#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus/mux.h"
#include "dbg/chk.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_HOST_NUM 4

#define AXI4_FIFO_DEPTH   16
#define AXI_MUX_TB_STG_FIFO_DEPTH 8
#define AXI_MUX_TB_OST_DEPTH 8
#define STRESS_TOTAL     1000

typedef struct axi_mux_tb {
    mod_t mod;
    u64 *cycle;
    u64  cycle_val;

    itf_t h0_axi4_aw_itf;
    itf_t h0_axi4_w_itf;
    itf_t h0_axi4_b_itf;
    itf_t h0_axi4_ar_itf;
    itf_t h0_axi4_r_itf;

    itf_t h1_axi4_aw_itf;
    itf_t h1_axi4_w_itf;
    itf_t h1_axi4_b_itf;
    itf_t h1_axi4_ar_itf;
    itf_t h1_axi4_r_itf;

    itf_t h2_axi4_aw_itf;
    itf_t h2_axi4_w_itf;
    itf_t h2_axi4_b_itf;
    itf_t h2_axi4_ar_itf;
    itf_t h2_axi4_r_itf;

    itf_t h3_axi4_aw_itf;
    itf_t h3_axi4_w_itf;
    itf_t h3_axi4_b_itf;
    itf_t h3_axi4_ar_itf;
    itf_t h3_axi4_r_itf;

    itf_t gst_axi4_aw_itf;
    itf_t gst_axi4_w_itf;
    itf_t gst_axi4_b_itf;
    itf_t gst_axi4_ar_itf;
    itf_t gst_axi4_r_itf;

    axi_mux_t dut;

    bool mock_ar_seen;
    u32  mock_ar_addr;
    u8   mock_ar_id;
    bool mock_aw_seen;
    u32  mock_aw_addr;
    u8   mock_aw_id;
    bool mock_w_seen;
    u32  mock_w_data;
    u8   mock_w_strb;
    bool mock_w_last;

    u32 mock_r_data;
    u8  mock_r_resp;
    u8  mock_b_resp;
    bool mock_auto_rsp;

    bool mock_aw_pending;
    u32  mock_pending_aw_addr;
    u8   mock_pending_aw_id;

    u32  mock_rd_order[STRESS_TOTAL + 16];
    u32  mock_rd_count;
    u32  mock_wr_order[STRESS_TOTAL + 16];
    u32  mock_wr_count;

    ut_sbd_t sbd;
} axi_mux_tb_t;

static void tb_construct(axi_mux_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    AXI4_AW_IF_CONSTRUCT(tb, h0_axi4_aw_itf, AXI4_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, h0_axi4_w_itf, AXI4_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, h0_axi4_b_itf, AXI4_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, h0_axi4_ar_itf, AXI4_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, h0_axi4_r_itf, AXI4_FIFO_DEPTH);

    AXI4_AW_IF_CONSTRUCT(tb, h1_axi4_aw_itf, AXI4_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, h1_axi4_w_itf, AXI4_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, h1_axi4_b_itf, AXI4_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, h1_axi4_ar_itf, AXI4_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, h1_axi4_r_itf, AXI4_FIFO_DEPTH);

    AXI4_AW_IF_CONSTRUCT(tb, h2_axi4_aw_itf, AXI4_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, h2_axi4_w_itf, AXI4_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, h2_axi4_b_itf, AXI4_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, h2_axi4_ar_itf, AXI4_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, h2_axi4_r_itf, AXI4_FIFO_DEPTH);

    AXI4_AW_IF_CONSTRUCT(tb, h3_axi4_aw_itf, AXI4_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, h3_axi4_w_itf, AXI4_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, h3_axi4_b_itf, AXI4_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, h3_axi4_ar_itf, AXI4_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, h3_axi4_r_itf, AXI4_FIFO_DEPTH);

    AXI4_AW_IF_CONSTRUCT(tb, gst_axi4_aw_itf, AXI4_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, gst_axi4_w_itf, AXI4_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, gst_axi4_b_itf, AXI4_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, gst_axi4_ar_itf, AXI4_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, gst_axi4_r_itf, AXI4_FIFO_DEPTH);

    tb->dut.host_axi4_aw_slvs[0] = &tb->h0_axi4_aw_itf;
    tb->dut.host_axi4_w_slvs[0]  = &tb->h0_axi4_w_itf;
    tb->dut.host_axi4_b_msts[0]  = &tb->h0_axi4_b_itf;
    tb->dut.host_axi4_ar_slvs[0] = &tb->h0_axi4_ar_itf;
    tb->dut.host_axi4_r_msts[0]  = &tb->h0_axi4_r_itf;

    tb->dut.host_axi4_aw_slvs[1] = &tb->h1_axi4_aw_itf;
    tb->dut.host_axi4_w_slvs[1]  = &tb->h1_axi4_w_itf;
    tb->dut.host_axi4_b_msts[1]  = &tb->h1_axi4_b_itf;
    tb->dut.host_axi4_ar_slvs[1] = &tb->h1_axi4_ar_itf;
    tb->dut.host_axi4_r_msts[1]  = &tb->h1_axi4_r_itf;

    tb->dut.host_axi4_aw_slvs[2] = &tb->h2_axi4_aw_itf;
    tb->dut.host_axi4_w_slvs[2]  = &tb->h2_axi4_w_itf;
    tb->dut.host_axi4_b_msts[2]  = &tb->h2_axi4_b_itf;
    tb->dut.host_axi4_ar_slvs[2] = &tb->h2_axi4_ar_itf;
    tb->dut.host_axi4_r_msts[2]  = &tb->h2_axi4_r_itf;

    tb->dut.host_axi4_aw_slvs[3] = &tb->h3_axi4_aw_itf;
    tb->dut.host_axi4_w_slvs[3]  = &tb->h3_axi4_w_itf;
    tb->dut.host_axi4_b_msts[3]  = &tb->h3_axi4_b_itf;
    tb->dut.host_axi4_ar_slvs[3] = &tb->h3_axi4_ar_itf;
    tb->dut.host_axi4_r_msts[3]  = &tb->h3_axi4_r_itf;

    tb->dut.gst_axi4_aw_mst = &tb->gst_axi4_aw_itf;
    tb->dut.gst_axi4_w_mst  = &tb->gst_axi4_w_itf;
    tb->dut.gst_axi4_b_slv  = &tb->gst_axi4_b_itf;
    tb->dut.gst_axi4_ar_mst = &tb->gst_axi4_ar_itf;
    tb->dut.gst_axi4_r_slv  = &tb->gst_axi4_r_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    axi_mux_conf_t conf = {
        .host_num = TB_HOST_NUM,
        .stg_fifo_depth = AXI_MUX_TB_STG_FIFO_DEPTH,
        .ost_depth = AXI_MUX_TB_OST_DEPTH
    };
    axi_mux_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(axi_mux_tb_t *tb)
{
    axi_mux_reset(&tb->dut);
    itf_reset(&tb->h0_axi4_aw_itf);
    itf_reset(&tb->h0_axi4_w_itf);
    itf_reset(&tb->h0_axi4_b_itf);
    itf_reset(&tb->h0_axi4_ar_itf);
    itf_reset(&tb->h0_axi4_r_itf);
    itf_reset(&tb->h1_axi4_aw_itf);
    itf_reset(&tb->h1_axi4_w_itf);
    itf_reset(&tb->h1_axi4_b_itf);
    itf_reset(&tb->h1_axi4_ar_itf);
    itf_reset(&tb->h1_axi4_r_itf);
    itf_reset(&tb->h2_axi4_aw_itf);
    itf_reset(&tb->h2_axi4_w_itf);
    itf_reset(&tb->h2_axi4_b_itf);
    itf_reset(&tb->h2_axi4_ar_itf);
    itf_reset(&tb->h2_axi4_r_itf);
    itf_reset(&tb->h3_axi4_aw_itf);
    itf_reset(&tb->h3_axi4_w_itf);
    itf_reset(&tb->h3_axi4_b_itf);
    itf_reset(&tb->h3_axi4_ar_itf);
    itf_reset(&tb->h3_axi4_r_itf);
    itf_reset(&tb->gst_axi4_aw_itf);
    itf_reset(&tb->gst_axi4_w_itf);
    itf_reset(&tb->gst_axi4_b_itf);
    itf_reset(&tb->gst_axi4_ar_itf);
    itf_reset(&tb->gst_axi4_r_itf);
    dbg_vcd_reset();

    tb->mock_ar_seen = false;
    tb->mock_aw_seen = false;
    tb->mock_w_seen = false;
    tb->mock_r_data = 0;
    tb->mock_r_resp = 0;
    tb->mock_b_resp = 0;
    tb->mock_auto_rsp = true;
    tb->mock_aw_pending = false;
    tb->mock_rd_count = 0;
    tb->mock_wr_count = 0;
}

static void tb_mock_axi4_guest(axi_mux_tb_t *tb)
{
    if (!tb->mock_auto_rsp) {
        return;
    }

    if (!itf_fifo_empty(&tb->gst_axi4_ar_itf) && !itf_fifo_full(&tb->gst_axi4_r_itf)) {
        axi4_ar_if_t ar;
        itf_read(&tb->gst_axi4_ar_itf, &ar);

        tb->mock_ar_seen = true;
        tb->mock_ar_addr = ar.addr;
        tb->mock_ar_id = ar.id;

        if (tb->mock_rd_count < STRESS_TOTAL + 16) {
            tb->mock_rd_order[tb->mock_rd_count++] = (u32)ar.id;
        }

        axi4_r_if_t r = {
            .id = ar.id,
            .data = tb->mock_r_data,
            .resp = tb->mock_r_resp,
            .last = true
        };
        itf_write(&tb->gst_axi4_r_itf, &r);
    }

    if (!itf_fifo_empty(&tb->gst_axi4_aw_itf) && !tb->mock_aw_pending) {
        axi4_aw_if_t aw;
        itf_read(&tb->gst_axi4_aw_itf, &aw);
        tb->mock_aw_pending = true;
        tb->mock_pending_aw_addr = aw.addr;
        tb->mock_pending_aw_id = aw.id;
    }

    if (tb->mock_aw_pending && !itf_fifo_empty(&tb->gst_axi4_w_itf) &&
        !itf_fifo_full(&tb->gst_axi4_b_itf)) {
        axi4_w_if_t w;
        itf_read(&tb->gst_axi4_w_itf, &w);

        tb->mock_aw_seen = true;
        tb->mock_aw_addr = tb->mock_pending_aw_addr;
        tb->mock_aw_id = tb->mock_pending_aw_id;
        tb->mock_w_seen = true;
        tb->mock_w_data = w.data;
        tb->mock_w_strb = w.strb;
        tb->mock_w_last = w.last;

        if (tb->mock_wr_count < STRESS_TOTAL + 16) {
            tb->mock_wr_order[tb->mock_wr_count++] = (u32)tb->mock_pending_aw_id;
        }

        axi4_b_if_t b = {
            .id = tb->mock_pending_aw_id,
            .resp = tb->mock_b_resp
        };
        itf_write(&tb->gst_axi4_b_itf, &b);
        tb->mock_aw_pending = false;
    }
}

static void tb_clock(axi_mux_tb_t *tb)
{
    axi_mux_clock(&tb->dut);
    tb_mock_axi4_guest(tb);

    itf_dbg_clock(&tb->h0_axi4_aw_itf);
    itf_dbg_clock(&tb->h0_axi4_w_itf);
    itf_dbg_clock(&tb->h0_axi4_b_itf);
    itf_dbg_clock(&tb->h0_axi4_ar_itf);
    itf_dbg_clock(&tb->h0_axi4_r_itf);

    itf_dbg_clock(&tb->h1_axi4_aw_itf);
    itf_dbg_clock(&tb->h1_axi4_w_itf);
    itf_dbg_clock(&tb->h1_axi4_b_itf);
    itf_dbg_clock(&tb->h1_axi4_ar_itf);
    itf_dbg_clock(&tb->h1_axi4_r_itf);

    itf_dbg_clock(&tb->h2_axi4_aw_itf);
    itf_dbg_clock(&tb->h2_axi4_w_itf);
    itf_dbg_clock(&tb->h2_axi4_b_itf);
    itf_dbg_clock(&tb->h2_axi4_ar_itf);
    itf_dbg_clock(&tb->h2_axi4_r_itf);

    itf_dbg_clock(&tb->h3_axi4_aw_itf);
    itf_dbg_clock(&tb->h3_axi4_w_itf);
    itf_dbg_clock(&tb->h3_axi4_b_itf);
    itf_dbg_clock(&tb->h3_axi4_ar_itf);
    itf_dbg_clock(&tb->h3_axi4_r_itf);

    itf_dbg_clock(&tb->gst_axi4_aw_itf);
    itf_dbg_clock(&tb->gst_axi4_w_itf);
    itf_dbg_clock(&tb->gst_axi4_b_itf);
    itf_dbg_clock(&tb->gst_axi4_ar_itf);
    itf_dbg_clock(&tb->gst_axi4_r_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free(axi_mux_tb_t *tb)
{
    axi_mux_free(&tb->dut);

    itf_free(&tb->h0_axi4_aw_itf);
    itf_free(&tb->h0_axi4_w_itf);
    itf_free(&tb->h0_axi4_b_itf);
    itf_free(&tb->h0_axi4_ar_itf);
    itf_free(&tb->h0_axi4_r_itf);

    itf_free(&tb->h1_axi4_aw_itf);
    itf_free(&tb->h1_axi4_w_itf);
    itf_free(&tb->h1_axi4_b_itf);
    itf_free(&tb->h1_axi4_ar_itf);
    itf_free(&tb->h1_axi4_r_itf);

    itf_free(&tb->h2_axi4_aw_itf);
    itf_free(&tb->h2_axi4_w_itf);
    itf_free(&tb->h2_axi4_b_itf);
    itf_free(&tb->h2_axi4_ar_itf);
    itf_free(&tb->h2_axi4_r_itf);

    itf_free(&tb->h3_axi4_aw_itf);
    itf_free(&tb->h3_axi4_w_itf);
    itf_free(&tb->h3_axi4_b_itf);
    itf_free(&tb->h3_axi4_ar_itf);
    itf_free(&tb->h3_axi4_r_itf);

    itf_free(&tb->gst_axi4_aw_itf);
    itf_free(&tb->gst_axi4_w_itf);
    itf_free(&tb->gst_axi4_b_itf);
    itf_free(&tb->gst_axi4_ar_itf);
    itf_free(&tb->gst_axi4_r_itf);
}

static void tb_host_write_ar(axi_mux_tb_t *tb, u32 host_idx, u8 id, u32 addr)
{
    axi4_ar_if_t ar = {
        .id = id,
        .addr = addr,
        .len = 0,
        .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR,
        .lock = false,
        .cache = 0,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    itf_t *host_ar = tb->dut.host_axi4_ar_slvs[host_idx];
    itf_write(host_ar, &ar);
}

static void tb_host_write_aw_w(axi_mux_tb_t *tb, u32 host_idx, u8 id, u32 addr,
                                u32 data, u8 strb)
{
    axi4_aw_if_t aw = {
        .id = id,
        .addr = addr,
        .len = 0,
        .size = AXI4_AW_SIZE_B4,
        .burst = AXI4_AW_BURST_INCR,
        .lock = false,
        .cache = 0,
        .prot = 0,
        .qos = 0,
        .user = 0
    };
    axi4_w_if_t w = {
        .data = data,
        .strb = strb,
        .last = true
    };
    itf_t *host_aw = tb->dut.host_axi4_aw_slvs[host_idx];
    itf_t *host_w  = tb->dut.host_axi4_w_slvs[host_idx];
    itf_write(host_aw, &aw);
    itf_write(host_w, &w);
}

static bool tb_cond_mock_ar_seen(axi_mux_tb_t *tb) { return tb->mock_ar_seen; }
static bool tb_cond_mock_aw_seen(axi_mux_tb_t *tb) { return tb->mock_aw_seen; }

static u32 tb_fifo_count(itf_t *itf)
{
    return itf->ctx.fifo.pkt_num;
}

static bool tb_cond_two_gst_ar(axi_mux_tb_t *tb)
{
    return tb_fifo_count(&tb->gst_axi4_ar_itf) >= 2;
}

static bool tb_cond_two_gst_aw_w(axi_mux_tb_t *tb)
{
    return tb_fifo_count(&tb->gst_axi4_aw_itf) >= 2 &&
           tb_fifo_count(&tb->gst_axi4_w_itf) >= 2;
}

static bool tb_cond_rd_table_full(axi_mux_tb_t *tb)
{
    return tb_fifo_count(&tb->gst_axi4_ar_itf) >= AXI_MUX_TB_OST_DEPTH;
}

static bool tb_cond_host_r_ready(axi_mux_tb_t *tb, u32 host_idx)
{
    return !itf_fifo_empty(tb->dut.host_axi4_r_msts[host_idx]);
}
static bool tb_cond_host_b_ready(axi_mux_tb_t *tb, u32 host_idx)
{
    return !itf_fifo_empty(tb->dut.host_axi4_b_msts[host_idx]);
}

static bool tb_cond_host_r_ready_h0(axi_mux_tb_t *tb) { return tb_cond_host_r_ready(tb, 0); }
static bool tb_cond_host_r_ready_h1(axi_mux_tb_t *tb) { return tb_cond_host_r_ready(tb, 1); }
static bool tb_cond_host_r_ready_h2(axi_mux_tb_t *tb) { return tb_cond_host_r_ready(tb, 2); }
static bool tb_cond_host_r_ready_h3(axi_mux_tb_t *tb) { return tb_cond_host_r_ready(tb, 3); }
static bool tb_cond_host_b_ready_h0(axi_mux_tb_t *tb) { return tb_cond_host_b_ready(tb, 0); }
static bool tb_cond_host_b_ready_h1(axi_mux_tb_t *tb) { return tb_cond_host_b_ready(tb, 1); }

static bool tb_host_check_and_pop_r(axi_mux_tb_t *tb, u32 host_idx, u8 expected_id,
                                     u32 expected_data, u8 expected_resp)
{
    itf_t *host_r = tb->dut.host_axi4_r_msts[host_idx];
    if (itf_fifo_empty(host_r)) return false;
    axi4_r_if_t r;
    itf_read(host_r, &r);
    return (r.id == expected_id) && (r.data == expected_data) && (r.resp == expected_resp);
}

static bool tb_host_check_and_pop_b(axi_mux_tb_t *tb, u32 host_idx, u8 expected_id,
                                     u8 expected_resp)
{
    itf_t *host_b = tb->dut.host_axi4_b_msts[host_idx];
    if (itf_fifo_empty(host_b)) return false;
    axi4_b_if_t b;
    itf_read(host_b, &b);
    return (b.id == expected_id) && (b.resp == expected_resp);
}

static bool tb_pop_gst_ar(axi_mux_tb_t *tb, u8 expected_id, u32 expected_addr)
{
    if (itf_fifo_empty(&tb->gst_axi4_ar_itf)) {
        return false;
    }
    axi4_ar_if_t ar;
    itf_read(&tb->gst_axi4_ar_itf, &ar);
    return ar.id == expected_id && ar.addr == expected_addr;
}

static bool tb_pop_gst_aw(axi_mux_tb_t *tb, u8 expected_id, u32 expected_addr)
{
    if (itf_fifo_empty(&tb->gst_axi4_aw_itf)) {
        return false;
    }
    axi4_aw_if_t aw;
    itf_read(&tb->gst_axi4_aw_itf, &aw);
    return aw.id == expected_id && aw.addr == expected_addr;
}

static bool tb_pop_gst_w(axi_mux_tb_t *tb, u32 expected_data)
{
    if (itf_fifo_empty(&tb->gst_axi4_w_itf)) {
        return false;
    }
    axi4_w_if_t w;
    itf_read(&tb->gst_axi4_w_itf, &w);
    return w.data == expected_data && w.last;
}

static void tb_write_gst_r(axi_mux_tb_t *tb, u8 id, u32 data,
                            axi4_r_resp_t resp, bool last)
{
    axi4_r_if_t r = {
        .id = id,
        .data = data,
        .resp = resp,
        .last = last
    };
    itf_write(&tb->gst_axi4_r_itf, &r);
}

static void tb_write_gst_b(axi_mux_tb_t *tb, u8 id, axi4_b_resp_t resp)
{
    axi4_b_if_t b = {
        .id = id,
        .resp = resp
    };
    itf_write(&tb->gst_axi4_b_itf, &b);
}

TEST_CASE(axi_mux_tb_t, read_h0)
{
    TEST_BEGIN("Host0 Single Read");

    tb->mock_r_data = 0xdeadbeef;
    tb->mock_r_resp = 0;

    tb_host_write_ar(tb, 0, 0x01, 0x12345678);

    bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_ar_seen, UT_TIMEOUT);
    REQUIRE(got_ar, "mock saw AR on guest bus");
    REQUIRE(tb->mock_ar_addr == 0x12345678, "AR addr = 0x12345678");
    REQUIRE(tb->mock_ar_id == 0x01, "AR id = 0x01");

    bool got_r = RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(got_r, "host0 R received");
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x01, 0xdeadbeef, 0),
              "host0 R: id=0x01 data=0xdeadbeef resp=0");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, write_h0)
{
    TEST_BEGIN("Host0 Single Write");

    tb->mock_b_resp = 0;

    tb_host_write_aw_w(tb, 0, 0x02, 0x20001000, 0xcafebabe, 0x0f);

    bool got_aw = RUN_POLL_UNTIL(tb_cond_mock_aw_seen, UT_TIMEOUT);
    REQUIRE(got_aw, "mock saw AW on guest bus");
    REQUIRE(tb->mock_aw_addr == 0x20001000, "AW addr = 0x20001000");
    REQUIRE(tb->mock_aw_id == 0x02, "AW id = 0x02");
    REQUIRE(tb->mock_w_seen, "mock also saw W");
    REQUIRE(tb->mock_w_data == 0xcafebabe, "W data = 0xcafebabe");
    REQUIRE(tb->mock_w_strb == 0x0f, "W strb = 0x0f");

    bool got_b = RUN_POLL_UNTIL(tb_cond_host_b_ready_h0, UT_TIMEOUT);
    REQUIRE(got_b, "host0 B received");
    REQUIRE(tb_host_check_and_pop_b(tb, 0, 0x02, 0),
              "host0 B: id=0x02 resp=0");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, rr_four_hosts_read)
{
    TEST_BEGIN("Round-Robin: Four Hosts Read (simultaneous)");

    tb->mock_r_data = 0xaaaa5555;
    tb->mock_r_resp = 0;

    tb_host_write_ar(tb, 0, 0x00, 0x10000000);
    tb_host_write_ar(tb, 1, 0x01, 0x20000000);
    tb_host_write_ar(tb, 2, 0x02, 0x30000000);
    tb_host_write_ar(tb, 3, 0x03, 0x40000000);

    bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_ar_seen, UT_TIMEOUT);
    REQUIRE(got_ar, "1st AR seen (rr starts at host0)");
    REQUIRE(tb->mock_ar_addr == 0x10000000, "1st AR addr = 0x10000000 (host0)");

    bool got_r0 = RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(got_r0, "host0 R received");
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x00, 0xaaaa5555, 0), "host0 R: id=0x00");

    bool got_r1 = RUN_POLL_UNTIL(tb_cond_host_r_ready_h1, UT_TIMEOUT);
    REQUIRE(got_r1, "host1 R received (rr advanced)");
    REQUIRE(tb_host_check_and_pop_r(tb, 1, 0x01, 0xaaaa5555, 0), "host1 R: id=0x01");

    bool got_r2 = RUN_POLL_UNTIL(tb_cond_host_r_ready_h2, UT_TIMEOUT);
    REQUIRE(got_r2, "host2 R received (rr advanced)");
    REQUIRE(tb_host_check_and_pop_r(tb, 2, 0x02, 0xaaaa5555, 0), "host2 R: id=0x02");

    bool got_r3 = RUN_POLL_UNTIL(tb_cond_host_r_ready_h3, UT_TIMEOUT);
    REQUIRE(got_r3, "host3 R received (rr advanced)");
    REQUIRE(tb_host_check_and_pop_r(tb, 3, 0x03, 0xaaaa5555, 0), "host3 R: id=0x03");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, back_to_back_read)
{
    TEST_BEGIN("Back-to-Back Reads from Host0");

    tb->mock_r_data = 0x11111111;
    tb->mock_r_resp = 0;

    tb_host_write_ar(tb, 0, 0x20, 0x40000000);

    bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_ar_seen, UT_TIMEOUT);
    REQUIRE(got_ar, "1st AR seen");
    REQUIRE(tb->mock_ar_addr == 0x40000000, "1st AR addr = 0x40000000");

    bool got_r = RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(got_r, "1st R received");
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x20, 0x11111111, 0),
              "1st R: id=0x20 data=0x11111111");

    tb->mock_ar_seen = false;
    tb->mock_r_data = 0x22222222;

    tb_host_write_ar(tb, 0, 0x21, 0x50000000);

    got_ar = RUN_POLL_UNTIL(tb_cond_mock_ar_seen, UT_TIMEOUT);
    REQUIRE(got_ar, "2nd AR seen");
    REQUIRE(tb->mock_ar_addr == 0x50000000, "2nd AR addr = 0x50000000");

    got_r = RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(got_r, "2nd R received");
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x21, 0x22222222, 0),
              "2nd R: id=0x21 data=0x22222222");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, back_to_back_write)
{
    TEST_BEGIN("Back-to-Back Writes from Host0");

    tb->mock_b_resp = 0;

    tb_host_write_aw_w(tb, 0, 0x30, 0x30000000, 0xaaaaaaaa, 0x0f);

    bool got_aw = RUN_POLL_UNTIL(tb_cond_mock_aw_seen, UT_TIMEOUT);
    REQUIRE(got_aw, "1st AW+W seen");
    REQUIRE(tb->mock_aw_addr == 0x30000000, "1st AW addr = 0x30000000");

    bool got_b = RUN_POLL_UNTIL(tb_cond_host_b_ready_h0, UT_TIMEOUT);
    REQUIRE(got_b, "1st B received");
    REQUIRE(tb_host_check_and_pop_b(tb, 0, 0x30, 0), "1st B: id=0x30 resp=0");

    tb->mock_aw_seen = false;
    tb->mock_w_seen = false;

    tb_host_write_aw_w(tb, 0, 0x31, 0x40000000, 0xbbbbbbbb, 0x0f);

    got_aw = RUN_POLL_UNTIL(tb_cond_mock_aw_seen, UT_TIMEOUT);
    REQUIRE(got_aw, "2nd AW+W seen");
    REQUIRE(tb->mock_aw_addr == 0x40000000, "2nd AW addr = 0x40000000");

    got_b = RUN_POLL_UNTIL(tb_cond_host_b_ready_h0, UT_TIMEOUT);
    REQUIRE(got_b, "2nd B received");
    REQUIRE(tb_host_check_and_pop_b(tb, 0, 0x31, 0), "2nd B: id=0x31 resp=0");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, mixed_rw)
{
    TEST_BEGIN("Mixed: Host0 Write + Host1 Read (dual outstanding)");

    tb->mock_r_data = 0xfeedface;
    tb->mock_r_resp = 0;
    tb->mock_b_resp = 0;

    tb_host_write_aw_w(tb, 0, 0x40, 0x60000000, 0x12345678, 0x0f);
    tb_host_write_ar(tb, 1, 0x41, 0x70000000);

    bool got_aw = RUN_POLL_UNTIL(tb_cond_mock_aw_seen, UT_TIMEOUT);
    REQUIRE(got_aw, "host0 AW+W seen (write rr starts at host0)");
    REQUIRE(tb->mock_aw_addr == 0x60000000, "host0 AW addr = 0x60000000");
    REQUIRE(tb->mock_w_data == 0x12345678, "host0 W data = 0x12345678");

    bool got_b_h0 = RUN_POLL_UNTIL(tb_cond_host_b_ready_h0, UT_TIMEOUT);
    REQUIRE(got_b_h0, "host0 B received");
    REQUIRE(tb_host_check_and_pop_b(tb, 0, 0x40, 0), "host0 B: id=0x40 resp=0");

    bool got_r_h1 = RUN_POLL_UNTIL(tb_cond_host_r_ready_h1, UT_TIMEOUT);
    REQUIRE(got_r_h1, "host1 R received (read rr starts at host0, skips to host1)");
    REQUIRE(tb_host_check_and_pop_r(tb, 1, 0x41, 0xfeedface, 0),
              "host1 R: id=0x41 data=0xfeedface");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, concurrent_rd_wr)
{
    TEST_BEGIN("Concurrent: Read and Write from Same Host");

    tb->mock_r_data = 0x11223344;
    tb->mock_r_resp = 0;
    tb->mock_b_resp = 0;

    tb_host_write_aw_w(tb, 0, 0x50, 0x80000000, 0x55667788, 0x0f);
    tb_host_write_ar(tb, 0, 0x51, 0x90000000);

    bool got_aw = RUN_POLL_UNTIL(tb_cond_mock_aw_seen, UT_TIMEOUT);
    REQUIRE(got_aw, "AW+W from host0 seen");

    bool got_ar = RUN_POLL_UNTIL(tb_cond_mock_ar_seen, UT_TIMEOUT);
    REQUIRE(got_ar, "AR from host0 also seen (concurrent)");

    bool got_b = RUN_POLL_UNTIL(tb_cond_host_b_ready_h0, UT_TIMEOUT);
    REQUIRE(got_b, "host0 B received");
    REQUIRE(tb_host_check_and_pop_b(tb, 0, 0x50, 0), "host0 B: id=0x50");

    bool got_r = RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(got_r, "host0 R received");
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x51, 0x11223344, 0),
              "host0 R: id=0x51 data=0x11223344");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, read_same_id_order)
{
    TEST_BEGIN("Multi-Outstanding Read: Same ID Routed in Issue Order");

    tb->mock_auto_rsp = false;

    tb_host_write_ar(tb, 0, 0x60, 0x61000000);
    tb_host_write_ar(tb, 1, 0x60, 0x61000004);

    RUN_POLL_UNTIL(tb_cond_two_gst_ar, UT_TIMEOUT);
    REQUIRE(tb_pop_gst_ar(tb, 0x60, 0x61000000), "host0 AR issued first");
    REQUIRE(tb_pop_gst_ar(tb, 0x60, 0x61000004), "host1 same-ID AR issued second");

    tb_write_gst_r(tb, 0x60, 0xaaaa0000, AXI4_R_RESP_OKAY, true);
    RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x60, 0xaaaa0000, 0),
            "first same-ID R routes to host0");
    REQUIRE(!tb_cond_host_r_ready(tb, 1), "host1 still waits for second same-ID R");

    tb_write_gst_r(tb, 0x60, 0xbbbb0000, AXI4_R_RESP_OKAY, true);
    RUN_POLL_UNTIL(tb_cond_host_r_ready_h1, UT_TIMEOUT);
    REQUIRE(tb_host_check_and_pop_r(tb, 1, 0x60, 0xbbbb0000, 0),
            "second same-ID R routes to host1");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, read_different_id_reorder)
{
    TEST_BEGIN("Multi-Outstanding Read: Different IDs May Reorder");

    tb->mock_auto_rsp = false;

    tb_host_write_ar(tb, 0, 0x61, 0x62000000);
    tb_host_write_ar(tb, 1, 0x62, 0x62000004);

    RUN_POLL_UNTIL(tb_cond_two_gst_ar, UT_TIMEOUT);
    REQUIRE(tb_pop_gst_ar(tb, 0x61, 0x62000000), "host0 AR issued");
    REQUIRE(tb_pop_gst_ar(tb, 0x62, 0x62000004), "host1 AR issued");

    tb_write_gst_r(tb, 0x62, 0x62626262, AXI4_R_RESP_OKAY, true);
    RUN_POLL_UNTIL(tb_cond_host_r_ready_h1, UT_TIMEOUT);
    REQUIRE(tb_host_check_and_pop_r(tb, 1, 0x62, 0x62626262, 0),
            "younger different-ID R routes to host1 first");

    tb_write_gst_r(tb, 0x61, 0x61616161, AXI4_R_RESP_OKAY, true);
    RUN_POLL_UNTIL(tb_cond_host_r_ready_h0, UT_TIMEOUT);
    REQUIRE(tb_host_check_and_pop_r(tb, 0, 0x61, 0x61616161, 0),
            "older different-ID R routes to host0 later");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, read_table_full_delayed_free)
{
    TEST_BEGIN("Read OST Full Uses Next-Cycle Free");

    tb->mock_auto_rsp = false;

    for (u32 i = 0; i < AXI_MUX_TB_OST_DEPTH + 1u; i++) {
        tb_host_write_ar(tb, 0, 0x70, 0x70000000 + i * 4u);
    }

    RUN_POLL_UNTIL(tb_cond_rd_table_full, UT_TIMEOUT);
    REQUIRE(tb_fifo_count(&tb->gst_axi4_ar_itf) == AXI_MUX_TB_OST_DEPTH,
            "read table depth worth of ARs issued");
    REQUIRE(tb_fifo_count(&tb->h0_axi4_ar_itf) == 1,
            "one host AR remains back-pressured while table is full");

    REQUIRE(tb_pop_gst_ar(tb, 0x70, 0x70000000), "oldest AR popped for response");
    tb_write_gst_r(tb, 0x70, 0x70000000, AXI4_R_RESP_OKAY, true);
    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->h0_axi4_ar_itf) == 0,
            "held host AR is captured into mux ingress FIFO");
    REQUIRE(fifo_count(&tb->dut.host_ar_fifos[0]) == 1,
            "captured host AR waits in ingress until next ost clock");
    REQUIRE(tb_fifo_count(&tb->gst_axi4_ar_itf) == AXI_MUX_TB_OST_DEPTH - 1u,
            "same-cycle free does not issue captured AR to guest");

    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->h0_axi4_ar_itf) == 0,
            "host AR interface remains drained after slot frees");
    REQUIRE(fifo_count(&tb->dut.host_ar_fifos[0]) == 0,
            "captured host AR is issued after ost clock releases slot");
    REQUIRE(tb_fifo_count(&tb->gst_axi4_ar_itf) == AXI_MUX_TB_OST_DEPTH,
            "guest sees held AR after ost clock releases slot");
    TEST_END();
}

TEST_CASE(axi_mux_tb_t, write_same_id_order)
{
    TEST_BEGIN("Multi-Outstanding Write: Same ID B Routed in Issue Order");

    tb->mock_auto_rsp = false;

    tb_host_write_aw_w(tb, 0, 0x80, 0x80000000, 0x11111111, 0x0f);
    tb_host_write_aw_w(tb, 1, 0x80, 0x80000004, 0x22222222, 0x0f);

    RUN_POLL_UNTIL(tb_cond_two_gst_aw_w, UT_TIMEOUT);
    REQUIRE(tb_pop_gst_aw(tb, 0x80, 0x80000000), "host0 AW issued first");
    REQUIRE(tb_pop_gst_aw(tb, 0x80, 0x80000004), "host1 same-ID AW issued second");
    REQUIRE(tb_pop_gst_w(tb, 0x11111111), "host0 W issued first");
    REQUIRE(tb_pop_gst_w(tb, 0x22222222), "host1 W issued second");

    tb_write_gst_b(tb, 0x80, AXI4_B_RESP_OKAY);
    RUN_POLL_UNTIL(tb_cond_host_b_ready_h0, UT_TIMEOUT);
    REQUIRE(tb_host_check_and_pop_b(tb, 0, 0x80, 0),
            "first same-ID B routes to host0");
    REQUIRE(!tb_cond_host_b_ready(tb, 1), "host1 waits for second same-ID B");

    tb_write_gst_b(tb, 0x80, AXI4_B_RESP_OKAY);
    RUN_POLL_UNTIL(tb_cond_host_b_ready_h1, UT_TIMEOUT);
    REQUIRE(tb_host_check_and_pop_b(tb, 1, 0x80, 0),
            "second same-ID B routes to host1");
    TEST_END();
}

#define STRESS_PER_HOST (STRESS_TOTAL / TB_HOST_NUM)

TEST_CASE(axi_mux_tb_t, stress_1000_mixed)
{
    TEST_BEGIN("Stress: 1000 mixed RW, dual RR fairness check");

    tb->mock_r_data = 0x5a5a5a5a;
    tb->mock_r_resp = 0;
    tb->mock_b_resp = 0;

    u32 host_sent[TB_HOST_NUM] = {0};
    u32 host_rcvd[TB_HOST_NUM] = {0};
    u32 total_sent = 0;
    u32 total_rcvd = 0;
    u32 seq[TB_HOST_NUM] = {0};

    while (total_rcvd < STRESS_TOTAL) {
        for (u32 h = 0; h < TB_HOST_NUM; h++) {
            if (host_sent[h] >= STRESS_PER_HOST) continue;

            itf_t *host_ar = tb->dut.host_axi4_ar_slvs[h];
            itf_t *host_aw = tb->dut.host_axi4_aw_slvs[h];
            itf_t *host_w  = tb->dut.host_axi4_w_slvs[h];

            if (!itf_fifo_full(host_ar) && !itf_fifo_full(host_aw) &&
                !itf_fifo_full(host_w)) {
                u8 id = (u8)h;
                u32 addr = 0x80000000 + (h << 16) + seq[h];
                if (total_sent & 1) {
                    tb_host_write_aw_w(tb, h, id, addr,
                                       0x10000000 + host_sent[h], 0x0f);
                } else {
                    tb_host_write_ar(tb, h, id, addr);
                }
                host_sent[h]++;
                seq[h]++;
                total_sent++;
            }
        }

        for (u32 h = 0; h < TB_HOST_NUM; h++) {
            itf_t *host_r = tb->dut.host_axi4_r_msts[h];
            itf_t *host_b = tb->dut.host_axi4_b_msts[h];
            while (!itf_fifo_empty(host_r)) {
                axi4_r_if_t r;
                itf_read(host_r, &r);
                host_rcvd[h]++;
                total_rcvd++;
            }
            while (!itf_fifo_empty(host_b)) {
                axi4_b_if_t b;
                itf_read(host_b, &b);
                host_rcvd[h]++;
                total_rcvd++;
            }
        }

        tb_clock(tb);

        if (*tb->cycle > (u64)STRESS_TOTAL * 100) {
            REQUIRE(false, "stress test timeout");
            break;
        }
    }

    REQUIRE(total_rcvd == STRESS_TOTAL, "all 1000 transactions completed");
    REQUIRE(total_sent == STRESS_TOTAL, "all 1000 transactions sent");

    for (u32 h = 0; h < TB_HOST_NUM; h++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "host%u sent %u / rcvd %u",
                 h, host_sent[h], host_rcvd[h]);
        REQUIRE(host_sent[h] == STRESS_PER_HOST, msg);
        REQUIRE(host_rcvd[h] == STRESS_PER_HOST, msg);
    }

    u32 total_guest = tb->mock_rd_count + tb->mock_wr_count;
    REQUIRE(total_guest == STRESS_TOTAL,
              "guest saw exactly 1000 transactions (no packet loss)");

    u32 max_svc_diff = 0;
    for (u32 i = 0; i < TB_HOST_NUM; i++) {
        for (u32 j = i + 1; j < TB_HOST_NUM; j++) {
            u32 a = host_rcvd[i];
            u32 b = host_rcvd[j];
            u32 diff = a > b ? a - b : b - a;
            if (diff > max_svc_diff) max_svc_diff = diff;
        }
    }
    REQUIRE(max_svc_diff <= 2, "RR fairness: max service count diff <= 2");

    TEST_END();
}

int main()
{
    axi_mux_tb_t tb;

    tb_construct(&tb, "tb");
    tb_reset(&tb);

    TEST_RUN(read_h0);
    tb_reset(&tb);
    TEST_RUN(write_h0);
    tb_reset(&tb);
    TEST_RUN(rr_four_hosts_read);
    tb_reset(&tb);
    TEST_RUN(back_to_back_read);
    tb_reset(&tb);
    TEST_RUN(back_to_back_write);
    tb_reset(&tb);
    TEST_RUN(mixed_rw);
    tb_reset(&tb);
    TEST_RUN(concurrent_rd_wr);
    tb_reset(&tb);
    TEST_RUN(read_same_id_order);
    tb_reset(&tb);
    TEST_RUN(read_different_id_reorder);
    tb_reset(&tb);
    TEST_RUN(read_table_full_delayed_free);
    tb_reset(&tb);
    TEST_RUN(write_same_id_order);
    tb_reset(&tb);
    TEST_RUN(stress_1000_mixed);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);

    tb_free(&tb);

    return ret;
}
