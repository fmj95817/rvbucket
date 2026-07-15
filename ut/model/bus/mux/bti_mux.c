#include <stdio.h>
#include <string.h>
#include "bus/mux.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_HOST_NUM 3
#define TB_FIFO_DEPTH 8
#define TB_OST_DEPTH 4

typedef struct bti_mux_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t h0_bti_req_itf;
    itf_t h0_bti_rsp_itf;
    itf_t h1_bti_req_itf;
    itf_t h1_bti_rsp_itf;
    itf_t h2_bti_req_itf;
    itf_t h2_bti_rsp_itf;

    itf_t gst_bti_req_itf;
    itf_t gst_bti_rsp_itf;

    itf_t *host_req_itfs[TB_HOST_NUM];
    itf_t *host_rsp_itfs[TB_HOST_NUM];

    bti_mux_t dut;
    ut_sbd_t sbd;
} bti_mux_tb_t;

static u32 tb_fifo_count(itf_t *itf)
{
    return itf->ctx.fifo.pkt_num;
}

static void tb_construct(bti_mux_tb_t *tb, const char *name)
{
    memset(tb, 0, sizeof(*tb));

    DBG_VCD_MODULE_SCOPE(name);
    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, h0_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, h0_bti_rsp_itf, TB_FIFO_DEPTH);
    BTI_REQ_IF_CONSTRUCT(tb, h1_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, h1_bti_rsp_itf, TB_FIFO_DEPTH);
    BTI_REQ_IF_CONSTRUCT(tb, h2_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, h2_bti_rsp_itf, TB_FIFO_DEPTH);
    BTI_REQ_IF_CONSTRUCT(tb, gst_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, gst_bti_rsp_itf, TB_FIFO_DEPTH);

    tb->host_req_itfs[0] = &tb->h0_bti_req_itf;
    tb->host_req_itfs[1] = &tb->h1_bti_req_itf;
    tb->host_req_itfs[2] = &tb->h2_bti_req_itf;
    tb->host_rsp_itfs[0] = &tb->h0_bti_rsp_itf;
    tb->host_rsp_itfs[1] = &tb->h1_bti_rsp_itf;
    tb->host_rsp_itfs[2] = &tb->h2_bti_rsp_itf;

    for (u32 i = 0; i < TB_HOST_NUM; i++) {
        tb->dut.host_bti_req_slvs[i] = tb->host_req_itfs[i];
        tb->dut.host_bti_rsp_msts[i] = tb->host_rsp_itfs[i];
    }
    tb->dut.gst_bti_req_mst = &tb->gst_bti_req_itf;
    tb->dut.gst_bti_rsp_slv = &tb->gst_bti_rsp_itf;
    tb->dut.mod.cycle = tb->mod.cycle;
    bti_mux_conf_t conf = {
        .host_num = TB_HOST_NUM,
        .stg_fifo_depth = TB_FIFO_DEPTH,
        .ost_depth = TB_OST_DEPTH
    };
    bti_mux_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(bti_mux_tb_t *tb)
{
    bti_mux_reset(&tb->dut);
    itf_reset(&tb->h0_bti_req_itf);
    itf_reset(&tb->h0_bti_rsp_itf);
    itf_reset(&tb->h1_bti_req_itf);
    itf_reset(&tb->h1_bti_rsp_itf);
    itf_reset(&tb->h2_bti_req_itf);
    itf_reset(&tb->h2_bti_rsp_itf);
    itf_reset(&tb->gst_bti_req_itf);
    itf_reset(&tb->gst_bti_rsp_itf);
    dbg_vcd_reset();
}

static void tb_clock(bti_mux_tb_t *tb)
{
    bti_mux_clock(&tb->dut);

    itf_dbg_clock(&tb->h0_bti_req_itf);
    itf_dbg_clock(&tb->h0_bti_rsp_itf);
    itf_dbg_clock(&tb->h1_bti_req_itf);
    itf_dbg_clock(&tb->h1_bti_rsp_itf);
    itf_dbg_clock(&tb->h2_bti_req_itf);
    itf_dbg_clock(&tb->h2_bti_rsp_itf);
    itf_dbg_clock(&tb->gst_bti_req_itf);
    itf_dbg_clock(&tb->gst_bti_rsp_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free(bti_mux_tb_t *tb)
{
    bti_mux_free(&tb->dut);
    itf_free(&tb->h0_bti_req_itf);
    itf_free(&tb->h0_bti_rsp_itf);
    itf_free(&tb->h1_bti_req_itf);
    itf_free(&tb->h1_bti_rsp_itf);
    itf_free(&tb->h2_bti_req_itf);
    itf_free(&tb->h2_bti_rsp_itf);
    itf_free(&tb->gst_bti_req_itf);
    itf_free(&tb->gst_bti_rsp_itf);
}

static void tb_write_host_req(bti_mux_tb_t *tb, u32 host_idx, u16 trans_id,
    u32 addr, bti_req_cmd_t cmd)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = cmd,
        .addr = addr,
        .size = BTI_REQ_SIZE_B4,
        .data = 0x1000u + addr,
        .strobe = cmd == BTI_REQ_CMD_WRITE ? 0xf : 0
    };
    itf_write(tb->host_req_itfs[host_idx], &req);
}

static bti_req_if_t tb_read_gst_req(bti_mux_tb_t *tb)
{
    bti_req_if_t req;
    itf_read(&tb->gst_bti_req_itf, &req);
    return req;
}

static void tb_write_gst_rsp(bti_mux_tb_t *tb, u16 trans_id, u32 data)
{
    bti_rsp_if_t rsp = {
        .trans_id = trans_id,
        .data = data,
        .ok = true
    };
    itf_write(&tb->gst_bti_rsp_itf, &rsp);
}

static bti_rsp_if_t tb_read_host_rsp(bti_mux_tb_t *tb, u32 host_idx)
{
    bti_rsp_if_t rsp;
    itf_read(tb->host_rsp_itfs[host_idx], &rsp);
    return rsp;
}

TEST_CASE(bti_mux_tb_t, accepts_multiple_before_response)
{
    TEST_BEGIN("Accepts Multiple Before Response");
    tb_reset(tb);

    tb_write_host_req(tb, 0, 0x10, 0x1000, BTI_REQ_CMD_READ);
    tb_write_host_req(tb, 1, 0x11, 0x2000, BTI_REQ_CMD_READ);

    RUN_CYCLES(2);

    REQUIRE(tb_fifo_count(&tb->gst_bti_req_itf) == 2,
            "guest sees two requests before any response");
    REQUIRE(ostk_count(&tb->dut.ost) == 2, "mux tracks two outstanding requests");

    bti_req_if_t req = tb_read_gst_req(tb);
    REQUIRE(req.trans_id == 0x10 && req.addr == 0x1000,
            "first guest request comes from host0");
    req = tb_read_gst_req(tb);
    REQUIRE(req.trans_id == 0x11 && req.addr == 0x2000,
            "second guest request comes from host1");

    TEST_END();
}

TEST_CASE(bti_mux_tb_t, different_keys_return_out_of_global_order)
{
    TEST_BEGIN("Different Keys Return Out of Global Order");
    tb_reset(tb);

    tb_write_host_req(tb, 0, 0x01, 0x1000, BTI_REQ_CMD_READ);
    tb_write_host_req(tb, 1, 0x02, 0x2000, BTI_REQ_CMD_READ);
    RUN_CYCLES(2);
    (void)tb_read_gst_req(tb);
    (void)tb_read_gst_req(tb);

    tb_write_gst_rsp(tb, 0x02, 0xbbbb0002);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->h1_bti_rsp_itf) == 1, "host1 receives key 2 response first");
    REQUIRE(tb_fifo_count(&tb->h0_bti_rsp_itf) == 0, "host0 remains pending");
    bti_rsp_if_t rsp = tb_read_host_rsp(tb, 1);
    REQUIRE(rsp.trans_id == 0x02 && rsp.data == 0xbbbb0002,
            "host1 response payload preserved");

    tb_write_gst_rsp(tb, 0x01, 0xaaaa0001);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->h0_bti_rsp_itf) == 1, "host0 receives key 1 response");
    rsp = tb_read_host_rsp(tb, 0);
    REQUIRE(rsp.trans_id == 0x01 && rsp.data == 0xaaaa0001,
            "host0 response payload preserved");

    TEST_END();
}

TEST_CASE(bti_mux_tb_t, same_key_returns_in_issue_order)
{
    TEST_BEGIN("Same Key Returns in Issue Order");
    tb_reset(tb);

    tb_write_host_req(tb, 0, 0x07, 0x1000, BTI_REQ_CMD_READ);
    tb_write_host_req(tb, 1, 0x07, 0x2000, BTI_REQ_CMD_READ);
    RUN_CYCLES(2);
    (void)tb_read_gst_req(tb);
    (void)tb_read_gst_req(tb);

    tb_write_gst_rsp(tb, 0x07, 0x11110000);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->h0_bti_rsp_itf) == 1,
            "first same-key response routes to first issuer");
    REQUIRE(tb_fifo_count(&tb->h1_bti_rsp_itf) == 0,
            "second same-key issuer remains pending");
    bti_rsp_if_t rsp = tb_read_host_rsp(tb, 0);
    REQUIRE(rsp.trans_id == 0x07 && rsp.data == 0x11110000,
            "first same-key response payload preserved");

    tb_write_gst_rsp(tb, 0x07, 0x22220000);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->h1_bti_rsp_itf) == 1,
            "second same-key response routes to second issuer");
    rsp = tb_read_host_rsp(tb, 1);
    REQUIRE(rsp.trans_id == 0x07 && rsp.data == 0x22220000,
            "second same-key response payload preserved");

    TEST_END();
}

TEST_CASE(bti_mux_tb_t, table_full_backpressures_new_requests)
{
    TEST_BEGIN("Table Full Backpressures New Requests");
    tb_reset(tb);

    for (u32 i = 0; i < TB_OST_DEPTH + 1u; i++) {
        tb_write_host_req(tb, 0, (u16)i, 0x1000 + i * 4u, BTI_REQ_CMD_READ);
    }

    RUN_CYCLES(TB_OST_DEPTH + 2u);
    REQUIRE(tb_fifo_count(&tb->gst_bti_req_itf) == TB_OST_DEPTH,
            "guest receives only table-depth requests");
    REQUIRE(tb_fifo_count(&tb->h0_bti_req_itf) == 0,
            "held host request is captured into ingress FIFO");
    REQUIRE(fifo_count(&tb->dut.host_req_fifos[0]) == 1,
            "one request waits in mux ingress while table is full");
    REQUIRE(ostk_count(&tb->dut.ost) == TB_OST_DEPTH,
            "outstanding table is full");

    tb_write_gst_rsp(tb, 0, 0xaaa00000);
    RUN_CYCLES(1);

    REQUIRE(tb_fifo_count(&tb->h0_bti_rsp_itf) == 1,
            "response frees one outstanding slot");
    REQUIRE(fifo_count(&tb->dut.host_req_fifos[0]) == 1,
            "ingress request waits until next ost clock");
    REQUIRE(tb_fifo_count(&tb->gst_bti_req_itf) == TB_OST_DEPTH,
            "guest does not see blocked request in response cycle");

    RUN_CYCLES(1);

    REQUIRE(tb_fifo_count(&tb->h0_bti_req_itf) == 0,
            "host interface remains drained after slot frees");
    REQUIRE(fifo_count(&tb->dut.host_req_fifos[0]) == 0,
            "ingress request is issued after slot frees");
    REQUIRE(tb_fifo_count(&tb->gst_bti_req_itf) == TB_OST_DEPTH + 1u,
            "guest receives the previously blocked request");

    TEST_END();
}

int main(void)
{
    bti_mux_tb_t tb;
    tb_construct(&tb, "bti_mux_tb");

    TEST_RUN(accepts_multiple_before_response);
    TEST_RUN(different_keys_return_out_of_global_order);
    TEST_RUN(same_key_returns_in_issue_order);
    TEST_RUN(table_full_backpressures_new_requests);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
