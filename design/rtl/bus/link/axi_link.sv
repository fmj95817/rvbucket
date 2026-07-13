module axi_link #(
    parameter int FIFO_DEPTH = 2
)(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv host_axi4_aw_slv,
    axi4_w_if_t.slv  host_axi4_w_slv,
    axi4_b_if_t.mst  host_axi4_b_mst,
    axi4_ar_if_t.slv host_axi4_ar_slv,
    axi4_r_if_t.mst  host_axi4_r_mst,
    axi4_aw_if_t.mst gst_axi4_aw_mst,
    axi4_w_if_t.mst  gst_axi4_w_mst,
    axi4_b_if_t.slv  gst_axi4_b_slv,
    axi4_ar_if_t.mst gst_axi4_ar_mst,
    axi4_r_if_t.slv  gst_axi4_r_slv
);
    localparam int AW_DW = $bits(host_axi4_aw_slv.pkt);
    localparam int W_DW = $bits(host_axi4_w_slv.pkt);
    localparam int B_DW = $bits(host_axi4_b_mst.pkt);
    localparam int AR_DW = $bits(host_axi4_ar_slv.pkt);
    localparam int R_DW = $bits(host_axi4_r_mst.pkt);

    logic [AW_DW-1:0] aw_data;
    logic [W_DW-1:0] w_data;
    logic [B_DW-1:0] b_data;
    logic [AR_DW-1:0] ar_data;
    logic [R_DW-1:0] r_data;

    fifo #(
        .DW    (AW_DW),
        .DEPTH (FIFO_DEPTH)
    ) u_aw_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_axi4_aw_slv.vld),
        .wr_rdy  (host_axi4_aw_slv.rdy),
        .wr_data (host_axi4_aw_slv.pkt),
        .rd_vld  (gst_axi4_aw_mst.vld),
        .rd_rdy  (gst_axi4_aw_mst.rdy),
        .rd_data (aw_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (W_DW),
        .DEPTH (FIFO_DEPTH)
    ) u_w_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_axi4_w_slv.vld),
        .wr_rdy  (host_axi4_w_slv.rdy),
        .wr_data (host_axi4_w_slv.pkt),
        .rd_vld  (gst_axi4_w_mst.vld),
        .rd_rdy  (gst_axi4_w_mst.rdy),
        .rd_data (w_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (B_DW),
        .DEPTH (FIFO_DEPTH)
    ) u_b_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (gst_axi4_b_slv.vld),
        .wr_rdy  (gst_axi4_b_slv.rdy),
        .wr_data (gst_axi4_b_slv.pkt),
        .rd_vld  (host_axi4_b_mst.vld),
        .rd_rdy  (host_axi4_b_mst.rdy),
        .rd_data (b_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (AR_DW),
        .DEPTH (FIFO_DEPTH)
    ) u_ar_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_axi4_ar_slv.vld),
        .wr_rdy  (host_axi4_ar_slv.rdy),
        .wr_data (host_axi4_ar_slv.pkt),
        .rd_vld  (gst_axi4_ar_mst.vld),
        .rd_rdy  (gst_axi4_ar_mst.rdy),
        .rd_data (ar_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (R_DW),
        .DEPTH (FIFO_DEPTH)
    ) u_r_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (gst_axi4_r_slv.vld),
        .wr_rdy  (gst_axi4_r_slv.rdy),
        .wr_data (gst_axi4_r_slv.pkt),
        .rd_vld  (host_axi4_r_mst.vld),
        .rd_rdy  (host_axi4_r_mst.rdy),
        .rd_data (r_data),
        .empty   (),
        .full    ()
    );

    assign gst_axi4_aw_mst.pkt = aw_data;
    assign gst_axi4_w_mst.pkt = w_data;
    assign host_axi4_b_mst.pkt = b_data;
    assign gst_axi4_ar_mst.pkt = ar_data;
    assign host_axi4_r_mst.pkt = r_data;
endmodule
