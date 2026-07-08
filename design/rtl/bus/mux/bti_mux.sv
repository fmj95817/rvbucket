module bti_mux2(
    input logic clk,
    input logic rst_n,
    bti_req_if_t.slv host0_req_slv,
    bti_rsp_if_t.mst host0_rsp_mst,
    bti_req_if_t.slv host1_req_slv,
    bti_rsp_if_t.mst host1_rsp_mst,
    bti_req_if_t.mst gst_req_mst,
    bti_rsp_if_t.slv gst_rsp_slv
);
    logic pending;
    logic owner;
    logic rr;
    wire select1 = host1_req_slv.vld && (rr || !host0_req_slv.vld);
    wire req_hsk = gst_req_mst.vld && gst_req_mst.rdy;
    wire rsp_hsk = gst_rsp_slv.vld && gst_rsp_slv.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            owner <= 1'b0;
            rr <= 1'b0;
        end else begin
            if (!pending && req_hsk) begin
                pending <= 1'b1;
                owner <= select1;
                rr <= !select1;
            end else if (pending && rsp_hsk) begin
                pending <= 1'b0;
            end
        end
    end

    assign gst_req_mst.vld = !pending && (host0_req_slv.vld || host1_req_slv.vld);
    assign gst_req_mst.pkt = select1 ? host1_req_slv.pkt : host0_req_slv.pkt;
    assign host0_req_slv.rdy = !pending && !select1 && gst_req_mst.rdy;
    assign host1_req_slv.rdy = !pending && select1 && gst_req_mst.rdy;
    assign host0_rsp_mst.vld = pending && !owner && gst_rsp_slv.vld;
    assign host0_rsp_mst.pkt = gst_rsp_slv.pkt;
    assign host1_rsp_mst.vld = pending && owner && gst_rsp_slv.vld;
    assign host1_rsp_mst.pkt = gst_rsp_slv.pkt;
    assign gst_rsp_slv.rdy = owner ? host1_rsp_mst.rdy : host0_rsp_mst.rdy;
endmodule
