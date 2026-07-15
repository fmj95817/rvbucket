`include "itf/bti_req_if.svh"

module bti_demux #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter GST_SEL_AW = 8,
    parameter GST_NUM = 4,
    parameter logic [GST_SEL_AW-1:0] GST_SEL[GST_NUM],
    parameter int GST_AW[GST_NUM],
    parameter int STG_FIFO_DEPTH = 4,
    parameter int OST_DEPTH = 8,
    localparam int GST_IDX_W = GST_NUM <= 1 ? 1 : $clog2(GST_NUM),
    localparam int OST_KEY_W = GST_IDX_W + 16,
    localparam int OST_SLOT_W = $clog2(OST_DEPTH)
)(
    input logic      clk,
    input logic      rst_n,
    bti_req_if_t.slv host_bti_req_slv,
    bti_rsp_if_t.mst host_bti_rsp_mst,
    bti_req_if_t.mst gst_bti_req_msts[GST_NUM],
    bti_rsp_if_t.slv gst_bti_rsp_slvs[GST_NUM]
);
    typedef struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [BTI_AW-1:0] addr;
        bti_req_size_t size;
        logic [BTI_DW-1:0] data;
        logic [(BTI_DW/8)-1:0] strobe;
    } bti_req_pkt_t;

    typedef struct packed {
        logic [GST_IDX_W-1:0] gst_idx;
    } ost_ctx_t;

    localparam int REQ_DW = $bits(bti_req_pkt_t);
    localparam int OST_CTX_DW = $bits(ost_ctx_t);
    localparam int GST_ADDR_END_BIT = BTI_AW - GST_SEL_AW;

    logic req_fifo_rd_vld;
    logic req_fifo_rd_rdy;
    logic [REQ_DW-1:0] req_fifo_rd_data;
    bti_req_pkt_t req_pkt;
    logic [GST_NUM-1:0] req_sel;
    logic req_hit;
    logic [GST_IDX_W-1:0] req_gst_idx;
    logic [GST_NUM-1:0] gst_req_rdy;
    logic [GST_NUM-1:0] gst_rsp_vld;
    logic [15:0] gst_rsp_trans_id[GST_NUM];
    logic [BTI_DW-1:0] gst_rsp_data[GST_NUM];
    logic [GST_NUM-1:0] gst_rsp_ok;
    logic rsp_sel_vld;
    logic [GST_IDX_W-1:0] rsp_sel_idx;

    logic ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] ost_alloc_slot;
    logic ost_lookup_vld;
    logic [OST_SLOT_W-1:0] ost_lookup_slot;
    ost_ctx_t ost_lookup_ctx;
    logic ost_empty;
    logic ost_full;

    wire [OST_KEY_W-1:0] req_ost_key = {req_gst_idx, req_pkt.trans_id};
    wire [OST_KEY_W-1:0] rsp_ost_key =
        {rsp_sel_idx, gst_rsp_trans_id[rsp_sel_idx]};
    wire req_hsk = req_fifo_rd_vld && req_fifo_rd_rdy;
    wire rsp_hsk = host_bti_rsp_mst.vld && host_bti_rsp_mst.rdy;

    function automatic logic [GST_IDX_W-1:0] gst_idx(input int unsigned idx);
        gst_idx = idx[GST_IDX_W-1:0];
    endfunction

    fifo #(
        .DW    (REQ_DW),
        .DEPTH (STG_FIFO_DEPTH)
    ) u_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_bti_req_slv.vld),
        .wr_rdy  (host_bti_req_slv.rdy),
        .wr_data (host_bti_req_slv.pkt),
        .rd_vld  (req_fifo_rd_vld),
        .rd_rdy  (req_fifo_rd_rdy),
        .rd_data (req_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    assign req_pkt = req_fifo_rd_data;

    ostk #(
        .KEY_W (OST_KEY_W),
        .DW    (OST_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (req_hsk),
        .alloc_rdy    (ost_alloc_rdy),
        .alloc_key    (req_ost_key),
        .alloc_ctx    (ost_ctx_t'{gst_idx:req_gst_idx}),
        .alloc_slot   (ost_alloc_slot),
        .lookup_key   (rsp_ost_key),
        .lookup_vld   (ost_lookup_vld),
        .lookup_slot  (ost_lookup_slot),
        .lookup_ctx   (ost_lookup_ctx),
        .free_vld     (rsp_hsk),
        .free_slot    (ost_lookup_slot),
        .slot_wr_vld  (1'b0),
        .slot_wr_idx  ('0),
        .slot_wr_ctx  ('0),
        .slot_vld     (),
        .slot_key_flat(),
        .slot_ctx_flat(),
        .slot_seq_flat(),
        .empty        (ost_empty),
        .full         (ost_full)
    );

    always_comb begin
        req_hit = 1'b0;
        req_gst_idx = '0;
        for (int unsigned i = 0; i < GST_NUM; i++) begin
            if (!req_hit && req_sel[i]) begin
                req_hit = 1'b1;
                req_gst_idx = gst_idx(i);
            end
        end
    end

    always_comb begin
        rsp_sel_vld = 1'b0;
        rsp_sel_idx = '0;
        for (int unsigned i = 0; i < GST_NUM; i++) begin
            if (!rsp_sel_vld && gst_rsp_vld[i]) begin
                rsp_sel_vld = 1'b1;
                rsp_sel_idx = gst_idx(i);
            end
        end
    end

    assign req_fifo_rd_rdy = req_fifo_rd_vld && req_hit &&
        ost_alloc_rdy && gst_req_rdy[req_gst_idx];

    for (genvar i = 0; i < GST_NUM; i++) begin : gen_gst
        assign req_sel[i] =
            (req_pkt.addr[BTI_AW-1:GST_ADDR_END_BIT] == GST_SEL[i]) &&
            (req_pkt.addr[GST_ADDR_END_BIT-1:GST_AW[i]] ==
                {(GST_ADDR_END_BIT-GST_AW[i]){1'b0}});
        assign gst_req_rdy[i] = gst_bti_req_msts[i].rdy;
        assign gst_rsp_vld[i] = gst_bti_rsp_slvs[i].vld;
        assign gst_rsp_trans_id[i] = gst_bti_rsp_slvs[i].pkt.trans_id;
        assign gst_rsp_data[i] = gst_bti_rsp_slvs[i].pkt.data;
        assign gst_rsp_ok[i] = gst_bti_rsp_slvs[i].pkt.ok;
        assign gst_bti_req_msts[i].vld = req_fifo_rd_vld && req_hit &&
            req_gst_idx == gst_idx(i) && ost_alloc_rdy;
        assign gst_bti_req_msts[i].pkt = req_pkt;
        assign gst_bti_rsp_slvs[i].rdy = host_bti_rsp_mst.rdy &&
            rsp_sel_vld && rsp_sel_idx == gst_idx(i) && ost_lookup_vld;
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && req_fifo_rd_vld) begin
            assert (req_hit)
                else $fatal(1, "bti_demux request address misses all guests");
        end
        if (rst_n && rsp_sel_vld) begin
            assert (ost_lookup_vld)
                else $fatal(1, "bti_demux response has no OST entry");
            assert (ost_lookup_ctx.gst_idx == rsp_sel_idx)
                else $fatal(1, "bti_demux response route mismatch");
        end
    end
`endif

    assign host_bti_rsp_mst.vld = rsp_sel_vld && ost_lookup_vld;
    assign host_bti_rsp_mst.pkt.trans_id = gst_rsp_trans_id[rsp_sel_idx];
    assign host_bti_rsp_mst.pkt.data = gst_rsp_data[rsp_sel_idx];
    assign host_bti_rsp_mst.pkt.ok = gst_rsp_ok[rsp_sel_idx];

    wire unused = (|ost_alloc_slot) | ost_empty | ost_full;
endmodule
