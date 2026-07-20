module axi_mux2 #(
    parameter int STG_FIFO_DEPTH = 4,
    parameter int OST_DEPTH = 8,
    localparam int OST_SLOT_W = $clog2(OST_DEPTH)
)(
    input logic clk,
    input logic rst_n,
    axi4_aw_if_t.slv host0_axi4_aw_slv,
    axi4_w_if_t.slv  host0_axi4_w_slv,
    axi4_b_if_t.mst  host0_axi4_b_mst,
    axi4_ar_if_t.slv host0_axi4_ar_slv,
    axi4_r_if_t.mst  host0_axi4_r_mst,
    axi4_aw_if_t.slv host1_axi4_aw_slv,
    axi4_w_if_t.slv  host1_axi4_w_slv,
    axi4_b_if_t.mst  host1_axi4_b_mst,
    axi4_ar_if_t.slv host1_axi4_ar_slv,
    axi4_r_if_t.mst  host1_axi4_r_mst,
    axi4_aw_if_t.mst gst_axi4_aw_mst,
    axi4_w_if_t.mst  gst_axi4_w_mst,
    axi4_b_if_t.slv  gst_axi4_b_slv,
    axi4_ar_if_t.mst gst_axi4_ar_mst,
    axi4_r_if_t.slv  gst_axi4_r_slv
);
    localparam int AW_DW = $bits(host0_axi4_aw_slv.pkt);
    localparam int W_DW = $bits(host0_axi4_w_slv.pkt);
    localparam int AR_DW = $bits(host0_axi4_ar_slv.pkt);

    logic rd_rr;
    logic wr_rr;
    logic host0_aw_vld, host0_aw_rdy;
    logic host1_aw_vld, host1_aw_rdy;
    logic host0_w_vld, host0_w_rdy;
    logic host1_w_vld, host1_w_rdy;
    logic host0_ar_vld, host0_ar_rdy;
    logic host1_ar_vld, host1_ar_rdy;
    logic [AW_DW-1:0] host0_aw_data, host1_aw_data;
    logic [W_DW-1:0] host0_w_data, host1_w_data;
    logic [AR_DW-1:0] host0_ar_data, host1_ar_data;
    logic aw_sel1, ar_sel1;
    logic [AW_DW-1:0] aw_issue_data;
    logic [AR_DW-1:0] ar_issue_data;

    logic rd_alloc_rdy, rd_lookup_vld, rd_lookup_ctx;
    logic [OST_SLOT_W-1:0] rd_alloc_slot, rd_lookup_slot;
    logic wr_rsp_alloc_rdy, wr_rsp_lookup_vld, wr_rsp_lookup_ctx;
    logic [OST_SLOT_W-1:0] wr_rsp_alloc_slot, wr_rsp_lookup_slot;
    logic wr_data_alloc_rdy, wr_data_head_vld, wr_data_head_ctx;
    logic [OST_SLOT_W-1:0] wr_data_alloc_slot, wr_data_head_slot;

    wire aw_hsk = gst_axi4_aw_mst.vld && gst_axi4_aw_mst.rdy;
    wire w_hsk = gst_axi4_w_mst.vld && gst_axi4_w_mst.rdy;
    wire b_hsk = gst_axi4_b_slv.vld && gst_axi4_b_slv.rdy;
    wire ar_hsk = gst_axi4_ar_mst.vld && gst_axi4_ar_mst.rdy;
    wire r_hsk = gst_axi4_r_slv.vld && gst_axi4_r_slv.rdy;

    fifo #(.DW(AW_DW), .DEPTH(STG_FIFO_DEPTH)) u_host0_aw_fifo(
        .clk(clk), .rst_n(rst_n), .clear(1'b0),
        .wr_vld(host0_axi4_aw_slv.vld), .wr_rdy(host0_axi4_aw_slv.rdy),
        .wr_data(host0_axi4_aw_slv.pkt), .rd_vld(host0_aw_vld),
        .rd_rdy(host0_aw_rdy), .rd_data(host0_aw_data), .empty(), .full());
    fifo #(.DW(AW_DW), .DEPTH(STG_FIFO_DEPTH)) u_host1_aw_fifo(
        .clk(clk), .rst_n(rst_n), .clear(1'b0),
        .wr_vld(host1_axi4_aw_slv.vld), .wr_rdy(host1_axi4_aw_slv.rdy),
        .wr_data(host1_axi4_aw_slv.pkt), .rd_vld(host1_aw_vld),
        .rd_rdy(host1_aw_rdy), .rd_data(host1_aw_data), .empty(), .full());
    fifo #(.DW(W_DW), .DEPTH(STG_FIFO_DEPTH)) u_host0_w_fifo(
        .clk(clk), .rst_n(rst_n), .clear(1'b0),
        .wr_vld(host0_axi4_w_slv.vld), .wr_rdy(host0_axi4_w_slv.rdy),
        .wr_data(host0_axi4_w_slv.pkt), .rd_vld(host0_w_vld),
        .rd_rdy(host0_w_rdy), .rd_data(host0_w_data), .empty(), .full());
    fifo #(.DW(W_DW), .DEPTH(STG_FIFO_DEPTH)) u_host1_w_fifo(
        .clk(clk), .rst_n(rst_n), .clear(1'b0),
        .wr_vld(host1_axi4_w_slv.vld), .wr_rdy(host1_axi4_w_slv.rdy),
        .wr_data(host1_axi4_w_slv.pkt), .rd_vld(host1_w_vld),
        .rd_rdy(host1_w_rdy), .rd_data(host1_w_data), .empty(), .full());
    fifo #(.DW(AR_DW), .DEPTH(STG_FIFO_DEPTH)) u_host0_ar_fifo(
        .clk(clk), .rst_n(rst_n), .clear(1'b0),
        .wr_vld(host0_axi4_ar_slv.vld), .wr_rdy(host0_axi4_ar_slv.rdy),
        .wr_data(host0_axi4_ar_slv.pkt), .rd_vld(host0_ar_vld),
        .rd_rdy(host0_ar_rdy), .rd_data(host0_ar_data), .empty(), .full());
    fifo #(.DW(AR_DW), .DEPTH(STG_FIFO_DEPTH)) u_host1_ar_fifo(
        .clk(clk), .rst_n(rst_n), .clear(1'b0),
        .wr_vld(host1_axi4_ar_slv.vld), .wr_rdy(host1_axi4_ar_slv.rdy),
        .wr_data(host1_axi4_ar_slv.pkt), .rd_vld(host1_ar_vld),
        .rd_rdy(host1_ar_rdy), .rd_data(host1_ar_data), .empty(), .full());

    assign aw_sel1 = host1_aw_vld && (wr_rr || !host0_aw_vld);
    assign aw_issue_data = aw_sel1 ? host1_aw_data : host0_aw_data;
    assign gst_axi4_aw_mst.vld = wr_data_alloc_rdy && wr_rsp_alloc_rdy &&
        (host0_aw_vld || host1_aw_vld);
    assign gst_axi4_aw_mst.pkt = aw_issue_data;
    assign host0_aw_rdy = aw_hsk && !aw_sel1;
    assign host1_aw_rdy = aw_hsk && aw_sel1;

    assign gst_axi4_w_mst.vld = wr_data_head_vld &&
        (wr_data_head_ctx ? host1_w_vld : host0_w_vld);
    assign gst_axi4_w_mst.pkt = wr_data_head_ctx ? host1_w_data : host0_w_data;
    assign host0_w_rdy = w_hsk && !wr_data_head_ctx;
    assign host1_w_rdy = w_hsk && wr_data_head_ctx;

    assign ar_sel1 = host1_ar_vld && (rd_rr || !host0_ar_vld);
    assign ar_issue_data = ar_sel1 ? host1_ar_data : host0_ar_data;
    assign gst_axi4_ar_mst.vld = rd_alloc_rdy &&
        (host0_ar_vld || host1_ar_vld);
    assign gst_axi4_ar_mst.pkt = ar_issue_data;
    assign host0_ar_rdy = ar_hsk && !ar_sel1;
    assign host1_ar_rdy = ar_hsk && ar_sel1;

    assign host0_axi4_b_mst.vld = gst_axi4_b_slv.vld &&
        wr_rsp_lookup_vld && !wr_rsp_lookup_ctx;
    assign host1_axi4_b_mst.vld = gst_axi4_b_slv.vld &&
        wr_rsp_lookup_vld && wr_rsp_lookup_ctx;
    assign host0_axi4_b_mst.pkt = gst_axi4_b_slv.pkt;
    assign host1_axi4_b_mst.pkt = gst_axi4_b_slv.pkt;
    assign gst_axi4_b_slv.rdy = wr_rsp_lookup_vld &&
        (wr_rsp_lookup_ctx ? host1_axi4_b_mst.rdy : host0_axi4_b_mst.rdy);

    assign host0_axi4_r_mst.vld = gst_axi4_r_slv.vld &&
        rd_lookup_vld && !rd_lookup_ctx;
    assign host1_axi4_r_mst.vld = gst_axi4_r_slv.vld &&
        rd_lookup_vld && rd_lookup_ctx;
    assign host0_axi4_r_mst.pkt = gst_axi4_r_slv.pkt;
    assign host1_axi4_r_mst.pkt = gst_axi4_r_slv.pkt;
    assign gst_axi4_r_slv.rdy = rd_lookup_vld &&
        (rd_lookup_ctx ? host1_axi4_r_mst.rdy : host0_axi4_r_mst.rdy);

    ostk #(.KEY_W(8), .DW(1), .DEPTH(OST_DEPTH)) u_rd_ost(
        .clk(clk), .rst_n(rst_n), .alloc_vld(ar_hsk),
        .alloc_rdy(rd_alloc_rdy), .alloc_key(ar_issue_data[AW_DW-1 -: 8]),
        .alloc_ctx(ar_sel1), .alloc_slot(rd_alloc_slot),
        .lookup_key(gst_axi4_r_slv.pkt.id), .lookup_vld(rd_lookup_vld),
        .lookup_slot(rd_lookup_slot), .lookup_ctx(rd_lookup_ctx),
        .free_vld(r_hsk && gst_axi4_r_slv.pkt.last),
        .free_slot(rd_lookup_slot), .slot_wr_vld(1'b0), .slot_wr_idx('0),
        .slot_wr_ctx('0), .slot_vld(), .slot_key_flat(), .slot_ctx_flat(),
        .slot_older_flat(), .empty(), .full());
    ostq #(.DW(1), .DEPTH(OST_DEPTH)) u_wr_data_ost(
        .clk(clk), .rst_n(rst_n), .alloc_vld(aw_hsk),
        .alloc_rdy(wr_data_alloc_rdy), .alloc_ctx(aw_sel1),
        .alloc_slot(wr_data_alloc_slot), .head_vld(wr_data_head_vld),
        .head_ctx(wr_data_head_ctx), .head_slot(wr_data_head_slot),
        .free_head(w_hsk && gst_axi4_w_mst.pkt.last),
        .slot_wr_vld(1'b0), .slot_wr_idx('0), .slot_wr_ctx('0),
        .slot_vld(), .slot_ctx_flat(), .empty(), .full());
    ostk #(.KEY_W(8), .DW(1), .DEPTH(OST_DEPTH)) u_wr_rsp_ost(
        .clk(clk), .rst_n(rst_n), .alloc_vld(aw_hsk),
        .alloc_rdy(wr_rsp_alloc_rdy), .alloc_key(aw_issue_data[AW_DW-1 -: 8]),
        .alloc_ctx(aw_sel1), .alloc_slot(wr_rsp_alloc_slot),
        .lookup_key(gst_axi4_b_slv.pkt.id), .lookup_vld(wr_rsp_lookup_vld),
        .lookup_slot(wr_rsp_lookup_slot), .lookup_ctx(wr_rsp_lookup_ctx),
        .free_vld(b_hsk), .free_slot(wr_rsp_lookup_slot),
        .slot_wr_vld(1'b0), .slot_wr_idx('0), .slot_wr_ctx('0),
        .slot_vld(), .slot_key_flat(), .slot_ctx_flat(), .slot_older_flat(),
        .empty(), .full());

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_rr <= 1'b0;
            wr_rr <= 1'b0;
        end else begin
            if (ar_hsk) rd_rr <= !ar_sel1;
            if (aw_hsk) wr_rr <= !aw_sel1;
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && gst_axi4_r_slv.vld)
            assert (rd_lookup_vld) else $fatal(1, "axi_mux2 R without owner");
        if (rst_n && gst_axi4_b_slv.vld)
            assert (wr_rsp_lookup_vld) else $fatal(1, "axi_mux2 B without owner");
    end
`endif

    wire unused = (|rd_alloc_slot) | (|wr_rsp_alloc_slot) |
        (|wr_data_alloc_slot) | (|wr_data_head_slot);
endmodule
