`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi_rsp_reg_slice(
    input logic       clk,
    input logic       rst_n,
    axi4_b_if_t.mst   host_axi4_b_mst,
    axi4_r_if_t.mst   host_axi4_r_mst,
    axi4_b_if_t.slv   gst_axi4_b_slv,
    axi4_r_if_t.slv   gst_axi4_r_slv
);
    localparam int B_DW = $bits(gst_axi4_b_slv.pkt);
    localparam int R_DW = $bits(gst_axi4_r_slv.pkt);

    logic [B_DW-1:0] b_out_data;
    logic [R_DW-1:0] r_out_data;

    bidir_reg_slice #(
        .DW (B_DW)
    ) u_b_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (gst_axi4_b_slv.vld),
        .src_rdy  (gst_axi4_b_slv.rdy),
        .src_data (gst_axi4_b_slv.pkt),
        .dst_vld  (host_axi4_b_mst.vld),
        .dst_rdy  (host_axi4_b_mst.rdy),
        .dst_data (b_out_data)
    );

    bidir_reg_slice #(
        .DW (R_DW)
    ) u_r_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (gst_axi4_r_slv.vld),
        .src_rdy  (gst_axi4_r_slv.rdy),
        .src_data (gst_axi4_r_slv.pkt),
        .dst_vld  (host_axi4_r_mst.vld),
        .dst_rdy  (host_axi4_r_mst.rdy),
        .dst_data (r_out_data)
    );

    assign host_axi4_b_mst.pkt = b_out_data;
    assign host_axi4_r_mst.pkt = r_out_data;
endmodule
