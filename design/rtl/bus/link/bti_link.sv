module bti_link #(
    parameter bit REQ_REG = 1'b1
)(
    input logic          clk,
    input logic          rst_n,
    bti_req_if_t.slv host_bti_req_slv,
    bti_rsp_if_t.mst host_bti_rsp_mst,
    bti_req_if_t.mst gst_bti_req_mst,
    bti_rsp_if_t.slv gst_bti_rsp_slv
);
    localparam int REQ_DW = $bits(host_bti_req_slv.pkt);
    logic [REQ_DW-1:0] req_data;

    if (REQ_REG) begin : gen_req_reg
        bidir_reg_slice #(
            .DW (REQ_DW)
        ) u_req_reg_slice(
            .clk      (clk),
            .rst_n    (rst_n),
            .clear    (1'b0),
            .src_vld  (host_bti_req_slv.vld),
            .src_rdy  (host_bti_req_slv.rdy),
            .src_data (host_bti_req_slv.pkt),
            .dst_vld  (gst_bti_req_mst.vld),
            .dst_rdy  (gst_bti_req_mst.rdy),
            .dst_data (req_data)
        );
    end else begin : gen_req_bypass
        assign gst_bti_req_mst.vld = host_bti_req_slv.vld;
        assign host_bti_req_slv.rdy = gst_bti_req_mst.rdy;
        assign req_data = host_bti_req_slv.pkt;
    end

    assign gst_bti_req_mst.pkt = req_data;
    assign host_bti_rsp_mst.vld = gst_bti_rsp_slv.vld;
    assign host_bti_rsp_mst.pkt = gst_bti_rsp_slv.pkt;
    assign gst_bti_rsp_slv.rdy = host_bti_rsp_mst.rdy;
endmodule
