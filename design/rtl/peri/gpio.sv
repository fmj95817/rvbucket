module gpio(
    input logic      clk,
    input logic      rst_n,
    bti_req_if_t.slv bti_req_slv,
    bti_rsp_if_t.mst bti_rsp_mst
);
    logic pending;
    logic [15:0] trans_id;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            trans_id <= '0;
        end else begin
            if (bti_req_slv.vld && bti_req_slv.rdy) begin
                pending <= 1'b1;
                trans_id <= bti_req_slv.pkt.trans_id;
            end else if (bti_rsp_mst.vld && bti_rsp_mst.rdy) begin
                pending <= 1'b0;
            end
        end
    end

    assign bti_req_slv.rdy = !pending;
    assign bti_rsp_mst.vld = pending;
    assign bti_rsp_mst.pkt.trans_id = trans_id;
    assign bti_rsp_mst.pkt.data = '0;
    assign bti_rsp_mst.pkt.ok = 1'b1;
endmodule
