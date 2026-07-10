#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus/demux.h"
#include "dbg/vcd.h"
#include "utils.h"

#define AXI_DEMUX_TB_FIFO_DEPTH 16
#define AXI_DEMUX_TB_STG_FIFO_DEPTH 8
#define AXI_DEMUX_TB_OST_DEPTH 8

typedef struct axi_demux_tb {
    mod_t mod;
    u64 *cycle;
    u64 cycle_val;

    itf_t host_aw;
    itf_t host_w;
    itf_t host_b;
    itf_t host_ar;
    itf_t host_r;

    itf_t gst0_aw;
    itf_t gst0_w;
    itf_t gst0_b;
    itf_t gst0_ar;
    itf_t gst0_r;
    itf_t gst1_aw;
    itf_t gst1_w;
    itf_t gst1_b;
    itf_t gst1_ar;
    itf_t gst1_r;

    axi_demux_t dut;
    ut_sbd_t sbd;
} axi_demux_tb_t;

static void tb_construct(axi_demux_tb_t *tb, const char *name)
{
    DBG_VCD_MODULE_SCOPE(name);

    tb->cycle_val = 0;
    tb->cycle = &tb->cycle_val;
    tb->mod.cycle = tb->cycle;
    mod_construct(&tb->mod, NULL, name);
    dbg_vcd_set_clk(tb->mod.cycle);

    AXI4_AW_IF_CONSTRUCT(tb, host_aw, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, host_w, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, host_b, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, host_ar, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, host_r, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_AW_IF_CONSTRUCT(tb, gst0_aw, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, gst0_w, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, gst0_b, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, gst0_ar, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, gst0_r, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_AW_IF_CONSTRUCT(tb, gst1_aw, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_W_IF_CONSTRUCT(tb, gst1_w, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_B_IF_CONSTRUCT(tb, gst1_b, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_AR_IF_CONSTRUCT(tb, gst1_ar, AXI_DEMUX_TB_FIFO_DEPTH);
    AXI4_R_IF_CONSTRUCT(tb, gst1_r, AXI_DEMUX_TB_FIFO_DEPTH);

    tb->dut.host_axi4_aw_slv = &tb->host_aw;
    tb->dut.host_axi4_w_slv = &tb->host_w;
    tb->dut.host_axi4_b_mst = &tb->host_b;
    tb->dut.host_axi4_ar_slv = &tb->host_ar;
    tb->dut.host_axi4_r_mst = &tb->host_r;
    tb->dut.gst_axi4_aw_msts[0] = &tb->gst0_aw;
    tb->dut.gst_axi4_w_msts[0] = &tb->gst0_w;
    tb->dut.gst_axi4_b_slvs[0] = &tb->gst0_b;
    tb->dut.gst_axi4_ar_msts[0] = &tb->gst0_ar;
    tb->dut.gst_axi4_r_slvs[0] = &tb->gst0_r;
    tb->dut.gst_axi4_aw_msts[1] = &tb->gst1_aw;
    tb->dut.gst_axi4_w_msts[1] = &tb->gst1_w;
    tb->dut.gst_axi4_b_slvs[1] = &tb->gst1_b;
    tb->dut.gst_axi4_ar_msts[1] = &tb->gst1_ar;
    tb->dut.gst_axi4_r_slvs[1] = &tb->gst1_r;
    const u32 bases[] = { 0x10000000, 0x20000000 };
    const u32 sizes[] = { 0x1000, 0x1000 };
    tb->dut.mod.cycle = tb->mod.cycle;
    axi_demux_conf_t conf = {
        .gst_num = 2,
        .gst_bases = bases,
        .gst_sizes = sizes,
        .stg_fifo_depth = AXI_DEMUX_TB_STG_FIFO_DEPTH,
        .ost_depth = AXI_DEMUX_TB_OST_DEPTH
    };
    axi_demux_construct(&tb->dut, tb->mod.hier_name, "u_dut", &conf);

    ut_sbd_init(&tb->sbd);
}

static void tb_reset(axi_demux_tb_t *tb)
{
    axi_demux_reset(&tb->dut);
    itf_reset(&tb->host_aw);
    itf_reset(&tb->host_w);
    itf_reset(&tb->host_b);
    itf_reset(&tb->host_ar);
    itf_reset(&tb->host_r);
    itf_reset(&tb->gst0_aw);
    itf_reset(&tb->gst0_w);
    itf_reset(&tb->gst0_b);
    itf_reset(&tb->gst0_ar);
    itf_reset(&tb->gst0_r);
    itf_reset(&tb->gst1_aw);
    itf_reset(&tb->gst1_w);
    itf_reset(&tb->gst1_b);
    itf_reset(&tb->gst1_ar);
    itf_reset(&tb->gst1_r);
    dbg_vcd_reset();
}

static void tb_clock(axi_demux_tb_t *tb)
{
    axi_demux_clock(&tb->dut);
    itf_dbg_clock(&tb->host_aw);
    itf_dbg_clock(&tb->host_w);
    itf_dbg_clock(&tb->host_b);
    itf_dbg_clock(&tb->host_ar);
    itf_dbg_clock(&tb->host_r);
    itf_dbg_clock(&tb->gst0_aw);
    itf_dbg_clock(&tb->gst0_w);
    itf_dbg_clock(&tb->gst0_b);
    itf_dbg_clock(&tb->gst0_ar);
    itf_dbg_clock(&tb->gst0_r);
    itf_dbg_clock(&tb->gst1_aw);
    itf_dbg_clock(&tb->gst1_w);
    itf_dbg_clock(&tb->gst1_b);
    itf_dbg_clock(&tb->gst1_ar);
    itf_dbg_clock(&tb->gst1_r);
    (*tb->cycle)++;
    dbg_vcd_clock();
}

static void tb_free(axi_demux_tb_t *tb)
{
    axi_demux_free(&tb->dut);
    itf_free(&tb->host_aw);
    itf_free(&tb->host_w);
    itf_free(&tb->host_b);
    itf_free(&tb->host_ar);
    itf_free(&tb->host_r);
    itf_free(&tb->gst0_aw);
    itf_free(&tb->gst0_w);
    itf_free(&tb->gst0_b);
    itf_free(&tb->gst0_ar);
    itf_free(&tb->gst0_r);
    itf_free(&tb->gst1_aw);
    itf_free(&tb->gst1_w);
    itf_free(&tb->gst1_b);
    itf_free(&tb->gst1_ar);
    itf_free(&tb->gst1_r);
}

static void tb_write_ar(axi_demux_tb_t *tb, u8 id, u32 addr, u8 len)
{
    axi4_ar_if_t ar = {
        .id = id, .addr = addr, .len = len, .size = AXI4_AR_SIZE_B4,
        .burst = AXI4_AR_BURST_INCR, .lock = false, .cache = 0,
        .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->host_ar, &ar);
}

static void tb_write_aw(axi_demux_tb_t *tb, u8 id, u32 addr, u8 len)
{
    axi4_aw_if_t aw = {
        .id = id, .addr = addr, .len = len, .size = AXI4_AW_SIZE_B4,
        .burst = AXI4_AW_BURST_INCR, .lock = false, .cache = 0,
        .prot = 0, .qos = 0, .user = 0
    };
    itf_write(&tb->host_aw, &aw);
}

static void tb_write_w(axi_demux_tb_t *tb, u32 data, bool last)
{
    axi4_w_if_t w = { .data = data, .strb = 0xf, .last = last };
    itf_write(&tb->host_w, &w);
}

static bool tb_cond_host_r(axi_demux_tb_t *tb)
{
    return !itf_fifo_empty(&tb->host_r);
}

static bool tb_cond_host_b(axi_demux_tb_t *tb)
{
    return !itf_fifo_empty(&tb->host_b);
}

static u32 tb_fifo_count(itf_t *itf)
{
    return itf->ctx.fifo.pkt_num;
}

static bool tb_cond_both_gst_ar(axi_demux_tb_t *tb)
{
    return !itf_fifo_empty(&tb->gst0_ar) &&
           !itf_fifo_empty(&tb->gst1_ar);
}

static bool tb_cond_both_gst_aw_w(axi_demux_tb_t *tb)
{
    return !itf_fifo_empty(&tb->gst0_aw) &&
           !itf_fifo_empty(&tb->gst1_aw) &&
           !itf_fifo_empty(&tb->gst0_w) &&
           !itf_fifo_empty(&tb->gst1_w);
}

static bool tb_cond_rd_ost_full_gst_ar(axi_demux_tb_t *tb)
{
    return tb_fifo_count(&tb->gst0_ar) >= AXI_DEMUX_TB_OST_DEPTH;
}

static bool tb_pop_gst_ar(itf_t *itf, u8 id, u32 addr)
{
    if (itf_fifo_empty(itf)) {
        return false;
    }

    axi4_ar_if_t ar;
    itf_read(itf, &ar);
    return ar.id == id && ar.addr == addr;
}

static bool tb_pop_gst_aw(itf_t *itf, u8 id, u32 addr)
{
    if (itf_fifo_empty(itf)) {
        return false;
    }

    axi4_aw_if_t aw;
    itf_read(itf, &aw);
    return aw.id == id && aw.addr == addr;
}

static bool tb_pop_gst_w(itf_t *itf, u32 data)
{
    if (itf_fifo_empty(itf)) {
        return false;
    }

    axi4_w_if_t w;
    itf_read(itf, &w);
    return w.data == data && w.last;
}

static bool tb_pop_host_r(axi_demux_tb_t *tb, u8 id, u32 data,
    axi4_r_resp_t resp, bool last)
{
    if (itf_fifo_empty(&tb->host_r)) {
        return false;
    }

    axi4_r_if_t r;
    itf_read(&tb->host_r, &r);
    return r.id == id && r.data == data && r.resp == resp && r.last == last;
}

static bool tb_pop_host_b(axi_demux_tb_t *tb, u8 id, axi4_b_resp_t resp)
{
    if (itf_fifo_empty(&tb->host_b)) {
        return false;
    }

    axi4_b_if_t b;
    itf_read(&tb->host_b, &b);
    return b.id == id && b.resp == resp;
}

TEST_CASE(axi_demux_tb_t, read_routes_by_address)
{
    TEST_BEGIN("Read Routes by Address");
    tb_reset(tb);

    tb_write_ar(tb, 3, 0x20000020, 1);
    RUN_CYCLES(1);

    REQUIRE(itf_fifo_empty(&tb->gst0_ar), "guest0 AR untouched");
    REQUIRE(!itf_fifo_empty(&tb->gst1_ar), "guest1 AR receives request");

    axi4_ar_if_t ar;
    itf_read(&tb->gst1_ar, &ar);
    REQUIRE(ar.id == 3 && ar.addr == 0x20000020 && ar.len == 1,
            "guest1 AR fields preserved");

    axi4_r_if_t r0 = { .id = 3, .data = 0x11111111, .resp = AXI4_R_RESP_OKAY, .last = false };
    axi4_r_if_t r1 = { .id = 3, .data = 0x22222222, .resp = AXI4_R_RESP_OKAY, .last = true };
    itf_write(&tb->gst1_r, &r0);
    itf_write(&tb->gst1_r, &r1);

    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    axi4_r_if_t got;
    itf_read(&tb->host_r, &got);
    REQUIRE(got.data == 0x11111111 && !got.last, "host receives first R beat");
    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    itf_read(&tb->host_r, &got);
    REQUIRE(got.data == 0x22222222 && got.last, "host receives last R beat");

    TEST_END();
}

TEST_CASE(axi_demux_tb_t, write_routes_w_and_b)
{
    TEST_BEGIN("Write Routes W and B");
    tb_reset(tb);

    tb_write_aw(tb, 7, 0x10000040, 1);
    tb_write_w(tb, 0xaaaa5555, false);
    tb_write_w(tb, 0x12345678, true);
    RUN_CYCLES(3);

    REQUIRE(!itf_fifo_empty(&tb->gst0_aw), "guest0 AW receives request");
    REQUIRE(itf_fifo_empty(&tb->gst1_aw), "guest1 AW untouched");
    REQUIRE(!itf_fifo_empty(&tb->gst0_w), "guest0 W receives data");
    REQUIRE(itf_fifo_empty(&tb->gst1_w), "guest1 W untouched");

    axi4_aw_if_t aw;
    axi4_w_if_t w;
    itf_read(&tb->gst0_aw, &aw);
    REQUIRE(aw.id == 7 && aw.addr == 0x10000040, "guest0 AW fields preserved");
    itf_read(&tb->gst0_w, &w);
    REQUIRE(w.data == 0xaaaa5555 && !w.last, "guest0 first W beat");
    itf_read(&tb->gst0_w, &w);
    REQUIRE(w.data == 0x12345678 && w.last, "guest0 last W beat");

    axi4_b_if_t b = { .id = 7, .resp = AXI4_B_RESP_OKAY };
    itf_write(&tb->gst0_b, &b);
    RUN_POLL_UNTIL(tb_cond_host_b, UT_TIMEOUT);
    axi4_b_if_t got;
    itf_read(&tb->host_b, &got);
    REQUIRE(got.id == 7 && got.resp == AXI4_B_RESP_OKAY, "host receives B from guest0");

    TEST_END();
}

TEST_CASE(axi_demux_tb_t, decode_miss_returns_error)
{
    TEST_BEGIN("Decode Miss Returns Error");
    tb_reset(tb);

    tb_write_ar(tb, 5, 0x30000000, 0);
    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    axi4_r_if_t r;
    itf_read(&tb->host_r, &r);
    REQUIRE(r.id == 5 && r.resp == AXI4_R_RESP_DECERR && r.last,
            "decode-miss read returns DECERR");

    tb_write_aw(tb, 6, 0x30000004, 0);
    tb_write_w(tb, 0xdeadbeef, true);
    RUN_POLL_UNTIL(tb_cond_host_b, UT_TIMEOUT);
    axi4_b_if_t b;
    itf_read(&tb->host_b, &b);
    REQUIRE(b.id == 6 && b.resp == AXI4_B_RESP_DECERR,
            "decode-miss write returns DECERR");

    TEST_END();
}

TEST_CASE(axi_demux_tb_t, read_same_id_order)
{
    TEST_BEGIN("Multi-Outstanding Read: Same ID Guest Order");
    tb_reset(tb);

    tb_write_ar(tb, 8, 0x10000010, 0);
    tb_write_ar(tb, 8, 0x20000010, 0);

    RUN_POLL_UNTIL(tb_cond_both_gst_ar, UT_TIMEOUT);
    REQUIRE(tb_pop_gst_ar(&tb->gst0_ar, 8, 0x10000010),
            "guest0 receives older same-ID AR");
    REQUIRE(tb_pop_gst_ar(&tb->gst1_ar, 8, 0x20000010),
            "guest1 receives younger same-ID AR");

    axi4_r_if_t r1 = {
        .id = 8, .data = 0x22222222, .resp = AXI4_R_RESP_OKAY, .last = true
    };
    itf_write(&tb->gst1_r, &r1);
    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->host_r) == 0,
            "younger same-ID R waits for older guest response");

    axi4_r_if_t r0 = {
        .id = 8, .data = 0x11111111, .resp = AXI4_R_RESP_OKAY, .last = true
    };
    itf_write(&tb->gst0_r, &r0);
    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    REQUIRE(tb_pop_host_r(tb, 8, 0x11111111, AXI4_R_RESP_OKAY, true),
            "older same-ID R returns first");

    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    REQUIRE(tb_pop_host_r(tb, 8, 0x22222222, AXI4_R_RESP_OKAY, true),
            "younger same-ID R returns second");

    TEST_END();
}

TEST_CASE(axi_demux_tb_t, read_different_id_reorder)
{
    TEST_BEGIN("Multi-Outstanding Read: Different IDs May Reorder");
    tb_reset(tb);

    tb_write_ar(tb, 9, 0x10000020, 0);
    tb_write_ar(tb, 10, 0x20000020, 0);

    RUN_POLL_UNTIL(tb_cond_both_gst_ar, UT_TIMEOUT);
    REQUIRE(tb_pop_gst_ar(&tb->gst0_ar, 9, 0x10000020),
            "guest0 receives older AR");
    REQUIRE(tb_pop_gst_ar(&tb->gst1_ar, 10, 0x20000020),
            "guest1 receives younger different-ID AR");

    axi4_r_if_t r1 = {
        .id = 10, .data = 0xaaaaaaaa, .resp = AXI4_R_RESP_OKAY, .last = true
    };
    itf_write(&tb->gst1_r, &r1);
    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    REQUIRE(tb_pop_host_r(tb, 10, 0xaaaaaaaa, AXI4_R_RESP_OKAY, true),
            "younger different-ID R may return first");

    axi4_r_if_t r0 = {
        .id = 9, .data = 0x99999999, .resp = AXI4_R_RESP_OKAY, .last = true
    };
    itf_write(&tb->gst0_r, &r0);
    RUN_POLL_UNTIL(tb_cond_host_r, UT_TIMEOUT);
    REQUIRE(tb_pop_host_r(tb, 9, 0x99999999, AXI4_R_RESP_OKAY, true),
            "older different-ID R returns when ready");

    TEST_END();
}

TEST_CASE(axi_demux_tb_t, read_ost_full_delayed_issue)
{
    TEST_BEGIN("Read OST Full Releases Slot Next Cycle");
    tb_reset(tb);

    for (u32 i = 0; i < AXI_DEMUX_TB_OST_DEPTH + 1u; i++) {
        tb_write_ar(tb, 11, 0x10000000 + i * 4u, 0);
    }

    RUN_POLL_UNTIL(tb_cond_rd_ost_full_gst_ar, UT_TIMEOUT);
    REQUIRE(tb_fifo_count(&tb->gst0_ar) == AXI_DEMUX_TB_OST_DEPTH,
            "read table depth worth of ARs reached guest0");
    REQUIRE(tb_fifo_count(&tb->host_ar) == 1,
            "one host AR remains before ingress capture");

    REQUIRE(tb_pop_gst_ar(&tb->gst0_ar, 11, 0x10000000),
            "oldest guest AR popped for response");
    axi4_r_if_t r = {
        .id = 11, .data = 0x10000000, .resp = AXI4_R_RESP_OKAY, .last = true
    };
    itf_write(&tb->gst0_r, &r);
    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->host_ar) == 0,
            "held host AR captured into ingress FIFO");
    REQUIRE(tb_fifo_count(&tb->gst0_ar) == AXI_DEMUX_TB_OST_DEPTH - 1u,
            "same-cycle free does not issue captured AR to guest");

    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->gst0_ar) == AXI_DEMUX_TB_OST_DEPTH,
            "captured AR issues after ost clock releases slot");

    TEST_END();
}

TEST_CASE(axi_demux_tb_t, write_same_id_order)
{
    TEST_BEGIN("Multi-Outstanding Write: Same ID B Order");
    tb_reset(tb);

    tb_write_aw(tb, 12, 0x10000030, 0);
    tb_write_w(tb, 0x12340000, true);
    tb_write_aw(tb, 12, 0x20000030, 0);
    tb_write_w(tb, 0x56780000, true);

    RUN_POLL_UNTIL(tb_cond_both_gst_aw_w, UT_TIMEOUT);
    REQUIRE(tb_pop_gst_aw(&tb->gst0_aw, 12, 0x10000030),
            "guest0 receives older same-ID AW");
    REQUIRE(tb_pop_gst_aw(&tb->gst1_aw, 12, 0x20000030),
            "guest1 receives younger same-ID AW");
    REQUIRE(tb_pop_gst_w(&tb->gst0_w, 0x12340000),
            "older W routed to guest0");
    REQUIRE(tb_pop_gst_w(&tb->gst1_w, 0x56780000),
            "younger W routed to guest1");

    axi4_b_if_t b1 = { .id = 12, .resp = AXI4_B_RESP_OKAY };
    itf_write(&tb->gst1_b, &b1);
    tb_clock(tb);
    REQUIRE(tb_fifo_count(&tb->host_b) == 0,
            "younger same-ID B waits for older guest response");

    axi4_b_if_t b0 = { .id = 12, .resp = AXI4_B_RESP_OKAY };
    itf_write(&tb->gst0_b, &b0);
    RUN_POLL_UNTIL(tb_cond_host_b, UT_TIMEOUT);
    REQUIRE(tb_pop_host_b(tb, 12, AXI4_B_RESP_OKAY),
            "older same-ID B returns first");

    RUN_POLL_UNTIL(tb_cond_host_b, UT_TIMEOUT);
    REQUIRE(tb_pop_host_b(tb, 12, AXI4_B_RESP_OKAY),
            "younger same-ID B returns second");

    TEST_END();
}

int main(void)
{
    axi_demux_tb_t tb;
    tb_construct(&tb, "axi_demux_tb");
    tb_reset(&tb);

    TEST_RUN(read_routes_by_address);
    TEST_RUN(write_routes_w_and_b);
    TEST_RUN(decode_miss_returns_error);
    TEST_RUN(read_same_id_order);
    TEST_RUN(read_different_id_reorder);
    TEST_RUN(read_ost_full_delayed_issue);
    TEST_RUN(write_same_id_order);

    ut_sbd_summary(&tb.sbd);
    int ret = ut_sbd_ret(&tb.sbd);
    tb_free(&tb);
    return ret;
}
