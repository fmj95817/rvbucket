`include "itf/bti_req_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi2bti(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  axi4_aw_slv,
    axi4_w_if_t.slv   axi4_w_slv,
    axi4_b_if_t.mst   axi4_b_mst,
    axi4_ar_if_t.slv  axi4_ar_slv,
    axi4_r_if_t.mst   axi4_r_mst,
    bti_req_if_t.mst  bti_req_mst,
    bti_rsp_if_t.slv  bti_rsp_slv
);
    logic pending;
    logic pending_write;
    logic [7:0] pending_id;
    wire write_req = axi4_aw_slv.vld && axi4_w_slv.vld;
    wire read_req = axi4_ar_slv.vld && !write_req;
    wire req_hsk = bti_req_mst.vld && bti_req_mst.rdy;
    wire rsp_hsk = bti_rsp_slv.vld && bti_rsp_slv.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            pending_write <= 1'b0;
            pending_id <= '0;
        end else begin
            if (!pending && req_hsk) begin
                pending <= 1'b1;
                pending_write <= write_req;
                pending_id <= write_req ? axi4_aw_slv.pkt.id : axi4_ar_slv.pkt.id;
            end else if (pending && rsp_hsk) begin
                pending <= 1'b0;
            end
        end
    end

    assign bti_req_mst.vld = !pending && (write_req || read_req);
    assign bti_req_mst.pkt.trans_id = '0;
    assign bti_req_mst.pkt.cmd = write_req ? BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
    assign bti_req_mst.pkt.addr = write_req ? axi4_aw_slv.pkt.addr : axi4_ar_slv.pkt.addr;
    assign bti_req_mst.pkt.size = write_req ?
        bti_req_size_t'(axi4_aw_slv.pkt.size) : bti_req_size_t'(axi4_ar_slv.pkt.size);
    assign bti_req_mst.pkt.data = axi4_w_slv.pkt.data;
    assign bti_req_mst.pkt.strobe = axi4_w_slv.pkt.strb;

    assign axi4_aw_slv.rdy = !pending && write_req && bti_req_mst.rdy;
    assign axi4_w_slv.rdy = !pending && write_req && bti_req_mst.rdy;
    assign axi4_ar_slv.rdy = !pending && read_req && bti_req_mst.rdy;

    assign axi4_r_mst.vld = pending && !pending_write && bti_rsp_slv.vld;
    assign axi4_r_mst.pkt.id = pending_id;
    assign axi4_r_mst.pkt.data = bti_rsp_slv.pkt.data;
    assign axi4_r_mst.pkt.resp = bti_rsp_slv.pkt.ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR;
    assign axi4_r_mst.pkt.last = 1'b1;

    assign axi4_b_mst.vld = pending && pending_write && bti_rsp_slv.vld;
    assign axi4_b_mst.pkt.id = pending_id;
    assign axi4_b_mst.pkt.resp = bti_rsp_slv.pkt.ok ? AXI4_B_RESP_OKAY : AXI4_B_RESP_SLVERR;
    assign bti_rsp_slv.rdy = pending_write ? axi4_b_mst.rdy : axi4_r_mst.rdy;
endmodule
