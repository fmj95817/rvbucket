module cfg_router(
    input logic clk, input logic rst_n,
    bti_req_if_t.slv host_req, bti_rsp_if_t.mst host_rsp,
    bti_req_if_t.mst peri_req, bti_rsp_if_t.slv peri_rsp,
    bti_req_if_t.mst aclint_req, bti_rsp_if_t.slv aclint_rsp,
    bti_req_if_t.mst plic_req, bti_rsp_if_t.slv plic_rsp
);
    typedef enum logic [1:0] {SEL_PERI, SEL_ACLINT, SEL_PLIC} sel_t;
    logic pending;
    sel_t sel, req_sel;
    always_comb begin
        if (host_req.pkt.addr[31:20] == 12'h310) req_sel = SEL_ACLINT;
        else if (host_req.pkt.addr[31:24] == 8'h31) req_sel = SEL_PLIC;
        else req_sel = SEL_PERI;
    end
    wire req_hsk = host_req.vld && host_req.rdy;
    wire rsp_hsk = host_rsp.vld && host_rsp.rdy;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin pending <= 0; sel <= SEL_PERI; end
        else if (!pending && req_hsk) begin pending <= 1; sel <= req_sel; end
        else if (pending && rsp_hsk) pending <= 0;
    end
    assign peri_req.vld = !pending && host_req.vld && req_sel == SEL_PERI;
    assign aclint_req.vld = !pending && host_req.vld && req_sel == SEL_ACLINT;
    assign plic_req.vld = !pending && host_req.vld && req_sel == SEL_PLIC;
    assign peri_req.pkt = host_req.pkt;
    assign aclint_req.pkt = host_req.pkt;
    assign plic_req.pkt = host_req.pkt;
    assign host_req.rdy = !pending && (req_sel == SEL_PERI ? peri_req.rdy : (req_sel == SEL_ACLINT ? aclint_req.rdy : plic_req.rdy));
    assign host_rsp.vld = sel == SEL_PERI ? peri_rsp.vld : (sel == SEL_ACLINT ? aclint_rsp.vld : plic_rsp.vld);
    assign host_rsp.pkt = sel == SEL_PERI ? peri_rsp.pkt : (sel == SEL_ACLINT ? aclint_rsp.pkt : plic_rsp.pkt);
    assign peri_rsp.rdy = pending && sel == SEL_PERI && host_rsp.rdy;
    assign aclint_rsp.rdy = pending && sel == SEL_ACLINT && host_rsp.rdy;
    assign plic_rsp.rdy = pending && sel == SEL_PLIC && host_rsp.rdy;
endmodule
