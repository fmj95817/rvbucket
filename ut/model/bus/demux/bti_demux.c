#include <stdio.h>
#include <string.h>
#include "bus/demux.h"
#include "dbg/vcd.h"
#include "utils.h"

#define TB_GST_NUM 2
#define TB_FIFO_DEPTH 8
#define TB_OST_DEPTH 4

typedef struct bti_demux_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t host_bti_req_itf;
    itf_t host_bti_rsp_itf;

    itf_t gst0_bti_req_itf;
    itf_t gst0_bti_rsp_itf;
    itf_t gst1_bti_req_itf;
    itf_t gst1_bti_rsp_itf;

    itf_t *gst_req_itfs[TB_GST_NUM];
    itf_t *gst_rsp_itfs[TB_GST_NUM];

    bti_demux_t dut;
    ut_sbd_t sbd;
} bti_demux_tb_t;

static u32 tb_fifo_count(itf_t *itf)
{
    return itf->ctx.fifo.pkt_num;
}

static void tb_construct(bti_demux_tb_t *tb, const char *name)
{
    memset(tb, 0, sizeof(*tb));

    DBG_VCD_MODULE_SCOPE(name);
    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    BTI_REQ_IF_CONSTRUCT(tb, host_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, host_bti_rsp_itf, TB_FIFO_DEPTH);
    BTI_REQ_IF_CONSTRUCT(tb, gst0_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, gst0_bti_rsp_itf, TB_FIFO_DEPTH);
    BTI_REQ_IF_CONSTRUCT(tb, gst1_bti_req_itf, TB_FIFO_DEPTH);
    BTI_RSP_IF_CONSTRUCT(tb, gst1_bti_rsp_itf, TB_FIFO_DEPTH);

    tb->gst_req_itfs[0] = &tb->gst0_bti_req_itf;
    tb->gst_req_itfs[1] = &tb->gst1_bti_req_itf;
    tb->gst_rsp_itfs[0] = &tb->gst0_bti_rsp_itf;
    tb->gst_rsp_itfs[1] = &tb->gst1_bti_rsp_itf;

    tb->dut.host_bti_req_slv = &tb->host_bti_req_itf;
    tb->dut.host_bti_rsp_mst = &tb->host_bti_rsp_itf;
    for (u32 i = 0; i < TB_GST_NUM; i++) {
        tb->dut.gst_bti_req_msts[i] = tb->gst_req_itfs[i];
        tb->dut.gst_bti_rsp_slvs[i] = tb->gst_rsp_itfs[i];
    }

    const u32 bases[] = { 0x10000000, 0x20000000 };
    const u32 sizes[] = { 0x1000, 0x1000 };
    tb->dut.mod.cycle = tb->mod.cycle;
    bti_demux_conf_t conf = {
        .gst_num = TB_GST_NUM,
        .gst_bases = bases,
        .gst_sizes = sizes,
        .stg_fifo_depth = TB_FIFO_DEPTH,
        .ost_depth = TB_OST_DEPTH
    };
    bti_demux_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(bti_demux_tb_t *tb)
{
    bti_demux_reset(&tb->dut);
    itf_reset(&tb->host_bti_req_itf);
    itf_reset(&tb->host_bti_rsp_itf);
    itf_reset(&tb->gst0_bti_req_itf);
    itf_reset(&tb->gst0_bti_rsp_itf);
    itf_reset(&tb->gst1_bti_req_itf);
    itf_reset(&tb->gst1_bti_rsp_itf);
    dbg_vcd_reset();
}

static void tb_clock(bti_demux_tb_t *tb)
{
    bti_demux_clock(&tb->dut);

    itf_dbg_clock(&tb->host_bti_req_itf);
    itf_dbg_clock(&tb->host_bti_rsp_itf);
    itf_dbg_clock(&tb->gst0_bti_req_itf);
    itf_dbg_clock(&tb->gst0_bti_rsp_itf);
    itf_dbg_clock(&tb->gst1_bti_req_itf);
    itf_dbg_clock(&tb->gst1_bti_rsp_itf);

    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free(bti_demux_tb_t *tb)
{
    bti_demux_free(&tb->dut);
    itf_free(&tb->host_bti_req_itf);
    itf_free(&tb->host_bti_rsp_itf);
    itf_free(&tb->gst0_bti_req_itf);
    itf_free(&tb->gst0_bti_rsp_itf);
    itf_free(&tb->gst1_bti_req_itf);
    itf_free(&tb->gst1_bti_rsp_itf);
}

static void tb_write_host_req(bti_demux_tb_t *tb, u16 trans_id, u32 addr)
{
    bti_req_if_t req = {
        .trans_id = trans_id,
        .cmd = BTI_REQ_CMD_READ,
        .addr = addr,
        .size = BTI_REQ_SIZE_B4,
        .data = 0,
        .strobe = 0
    };
    itf_write(&tb->host_bti_req_itf, &req);
}

static bti_req_if_t tb_read_gst_req(bti_demux_tb_t *tb, u32 gst_idx)
{
    bti_req_if_t req;
    itf_read(tb->gst_req_itfs[gst_idx], &req);
    return req;
}

static void tb_write_gst_rsp(bti_demux_tb_t *tb, u32 gst_idx, u16 trans_id, u32 data)
{
    bti_rsp_if_t rsp = {
        .trans_id = trans_id,
        .data = data,
        .ok = true
    };
    itf_write(tb->gst_rsp_itfs[gst_idx], &rsp);
}

static bti_rsp_if_t tb_read_host_rsp(bti_demux_tb_t *tb)
{
    bti_rsp_if_t rsp;
    itf_read(&tb->host_bti_rsp_itf, &rsp);
    return rsp;
}

TEST_CASE(bti_demux_tb_t, routes_multiple_before_response)
{
    TEST_BEGIN("Routes Multiple Before Response");
    tb_reset(tb);

    tb_write_host_req(tb, 0x10, 0x10000020);
    tb_write_host_req(tb, 0x11, 0x20000040);
    RUN_CYCLES(2);

    REQUIRE(tb_fifo_count(&tb->gst0_bti_req_itf) == 1,
            "guest0 receives first request");
    REQUIRE(tb_fifo_count(&tb->gst1_bti_req_itf) == 1,
            "guest1 receives second request");
    REQUIRE(ostk_count(&tb->dut.ost) == 2,
            "demux tracks two outstanding requests");

    bti_req_if_t req = tb_read_gst_req(tb, 0);
    REQUIRE(req.trans_id == 0x10 && req.addr == 0x10000020,
            "guest0 request fields preserved");
    req = tb_read_gst_req(tb, 1);
    REQUIRE(req.trans_id == 0x11 && req.addr == 0x20000040,
            "guest1 request fields preserved");

    TEST_END();
}

TEST_CASE(bti_demux_tb_t, guest_responses_interleave)
{
    TEST_BEGIN("Guest Responses Interleave");
    tb_reset(tb);

    tb_write_host_req(tb, 0x01, 0x10000000);
    tb_write_host_req(tb, 0x02, 0x20000000);
    RUN_CYCLES(2);
    (void)tb_read_gst_req(tb, 0);
    (void)tb_read_gst_req(tb, 1);

    tb_write_gst_rsp(tb, 1, 0x02, 0xbbbb0002);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->host_bti_rsp_itf) == 1,
            "host receives guest1 response first");
    bti_rsp_if_t rsp = tb_read_host_rsp(tb);
    REQUIRE(rsp.trans_id == 0x02 && rsp.data == 0xbbbb0002,
            "guest1 response fields preserved");

    tb_write_gst_rsp(tb, 0, 0x01, 0xaaaa0001);
    RUN_CYCLES(1);
    REQUIRE(tb_fifo_count(&tb->host_bti_rsp_itf) == 1,
            "host receives guest0 response second");
    rsp = tb_read_host_rsp(tb);
    REQUIRE(rsp.trans_id == 0x01 && rsp.data == 0xaaaa0001,
            "guest0 response fields preserved");

    TEST_END();
}

TEST_CASE(bti_demux_tb_t, same_guest_same_key_order)
{
    TEST_BEGIN("Same Guest Same Key Order");
    tb_reset(tb);

    tb_write_host_req(tb, 0x07, 0x10000000);
    tb_write_host_req(tb, 0x07, 0x10000004);
    RUN_CYCLES(2);
    (void)tb_read_gst_req(tb, 0);
    (void)tb_read_gst_req(tb, 0);

    tb_write_gst_rsp(tb, 0, 0x07, 0x11110000);
    RUN_CYCLES(1);
    bti_rsp_if_t rsp = tb_read_host_rsp(tb);
    REQUIRE(rsp.trans_id == 0x07 && rsp.data == 0x11110000,
            "first same-key response accepted");

    tb_write_gst_rsp(tb, 0, 0x07, 0x22220000);
    RUN_CYCLES(1);
    rsp = tb_read_host_rsp(tb);
    REQUIRE(rsp.trans_id == 0x07 && rsp.data == 0x22220000,
            "second same-key response accepted");
    REQUIRE(!ostk_empty(&tb->dut.ost), "same-key second free is pending until clock");
    RUN_CYCLES(1);
    REQUIRE(ostk_empty(&tb->dut.ost), "same-key entries freed in order");

    TEST_END();
}

TEST_CASE(bti_demux_tb_t, table_full_backpressures_host)
{
    TEST_BEGIN("Table Full Backpressures Host");
    tb_reset(tb);

    for (u32 i = 0; i < TB_OST_DEPTH + 1u; i++) {
        tb_write_host_req(tb, (u16)i, 0x10000000 + i * 4u);
    }

    RUN_CYCLES(TB_OST_DEPTH + 2u);
    REQUIRE(tb_fifo_count(&tb->gst0_bti_req_itf) == TB_OST_DEPTH,
            "guest receives only table-depth requests");
    REQUIRE(tb_fifo_count(&tb->host_bti_req_itf) == 0,
            "held host request is captured into ingress FIFO");
    REQUIRE(fifo_count(&tb->dut.req_fifo) == 1,
            "one request waits in demux ingress while table is full");
    REQUIRE(ostk_count(&tb->dut.ost) == TB_OST_DEPTH,
            "outstanding table is full");

    tb_write_gst_rsp(tb, 0, 0, 0xaaaa0000);
    RUN_CYCLES(1);

    REQUIRE(tb_fifo_count(&tb->host_bti_rsp_itf) == 1,
            "host receives response that frees one slot");
    REQUIRE(fifo_count(&tb->dut.req_fifo) == 1,
            "ingress request waits until next ost clock");
    REQUIRE(tb_fifo_count(&tb->gst0_bti_req_itf) == TB_OST_DEPTH,
            "guest does not see blocked request in response cycle");

    RUN_CYCLES(1);

    REQUIRE(tb_fifo_count(&tb->host_bti_req_itf) == 0,
            "host interface remains drained after slot frees");
    REQUIRE(fifo_count(&tb->dut.req_fifo) == 0,
            "ingress request is issued after slot frees");
    REQUIRE(tb_fifo_count(&tb->gst0_bti_req_itf) == TB_OST_DEPTH + 1u,
            "guest receives previously blocked request");

    TEST_END();
}

int main(void)
{
    bti_demux_tb_t tb;
    tb_construct(&tb, "bti_demux_tb");

    TEST_RUN(routes_multiple_before_response);
    TEST_RUN(guest_responses_interleave);
    TEST_RUN(same_guest_same_key_order);
    TEST_RUN(table_full_backpressures_host);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
