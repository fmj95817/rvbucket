`include "itf/bti_req_if.svh"
`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module bti2axi(
    input logic       clk,
    input logic       rst_n,
    bti_req_if_t.slv  bti_req_slv,
    bti_rsp_if_t.mst  bti_rsp_mst,
    axi4_aw_if_t.mst  axi4_aw_mst,
    axi4_w_if_t.mst   axi4_w_mst,
    axi4_b_if_t.slv   axi4_b_slv,
    axi4_ar_if_t.mst  axi4_ar_mst,
    axi4_r_if_t.slv   axi4_r_slv
);
    typedef enum logic [2:0] {
        IDLE,
        RD_SEND,
        RD_RESP,
        WR_SEND,
        WR_RESP
    } state_t;

    state_t state;
    logic [15:0] req_trans_id;
    logic [31:0] req_addr;
    bti_req_size_t req_size;
    logic [31:0] req_data;
    logic [3:0] req_strobe;
    logic aw_done;
    logic w_done;
    wire aw_hsk = axi4_aw_mst.vld && axi4_aw_mst.rdy;
    wire w_hsk = axi4_w_mst.vld && axi4_w_mst.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            req_trans_id <= '0;
            req_addr <= '0;
            req_size <= BTI_REQ_SIZE_B1;
            req_data <= '0;
            req_strobe <= '0;
            aw_done <= 1'b0;
            w_done <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    if (bti_req_slv.vld && bti_req_slv.rdy) begin
                        req_trans_id <= bti_req_slv.pkt.trans_id;
                        req_addr <= bti_req_slv.pkt.addr;
                        req_size <= bti_req_slv.pkt.size;
                        req_data <= bti_req_slv.pkt.data;
                        req_strobe <= bti_req_slv.pkt.strobe;
                        if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_READ)
                            state <= RD_SEND;
                        else begin
                            state <= WR_SEND;
                            aw_done <= 1'b0;
                            w_done <= 1'b0;
                        end
                    end
                end
                RD_SEND: begin
                    if (axi4_ar_mst.vld && axi4_ar_mst.rdy)
                        state <= RD_RESP;
                end
                RD_RESP: begin
                    if (axi4_r_slv.vld && axi4_r_slv.rdy)
                        state <= IDLE;
                end
                WR_SEND: begin
                    if (aw_hsk)
                        aw_done <= 1'b1;
                    if (w_hsk)
                        w_done <= 1'b1;
                    if ((aw_done || aw_hsk) && (w_done || w_hsk))
                        state <= WR_RESP;
                end
                WR_RESP: begin
                    if (axi4_b_slv.vld && axi4_b_slv.rdy)
                        state <= IDLE;
                end
                default: state <= IDLE;
            endcase
        end
    end

    assign bti_req_slv.rdy = state == IDLE;

    assign axi4_ar_mst.vld = state == RD_SEND;
    assign axi4_ar_mst.pkt.id = '0;
    assign axi4_ar_mst.pkt.addr = req_addr;
    assign axi4_ar_mst.pkt.len = '0;
    assign axi4_ar_mst.pkt.size = axi4_ar_size_t'(req_size);
    assign axi4_ar_mst.pkt.burst = AXI4_AR_BURST_FIXED;
    assign axi4_ar_mst.pkt.lock = 1'b0;
    assign axi4_ar_mst.pkt.cache = 4'h0;
    assign axi4_ar_mst.pkt.prot = 3'h0;
    assign axi4_ar_mst.pkt.qos = 4'h0;
    assign axi4_ar_mst.pkt.user = '0;

    assign axi4_aw_mst.vld = state == WR_SEND && !aw_done;
    assign axi4_aw_mst.pkt.id = '0;
    assign axi4_aw_mst.pkt.addr = req_addr;
    assign axi4_aw_mst.pkt.len = '0;
    assign axi4_aw_mst.pkt.size = axi4_aw_size_t'(req_size);
    assign axi4_aw_mst.pkt.burst = AXI4_AW_BURST_FIXED;
    assign axi4_aw_mst.pkt.lock = 1'b0;
    assign axi4_aw_mst.pkt.cache = 4'h0;
    assign axi4_aw_mst.pkt.prot = 3'h0;
    assign axi4_aw_mst.pkt.qos = 4'h0;
    assign axi4_aw_mst.pkt.user = '0;

    assign axi4_w_mst.vld = state == WR_SEND && !w_done;
    assign axi4_w_mst.pkt.data = req_data;
    assign axi4_w_mst.pkt.strb = req_strobe;
    assign axi4_w_mst.pkt.last = 1'b1;

    assign bti_rsp_mst.vld = state == RD_RESP ? axi4_r_slv.vld :
        (state == WR_RESP ? axi4_b_slv.vld : 1'b0);
    assign bti_rsp_mst.pkt.trans_id = req_trans_id;
    assign bti_rsp_mst.pkt.data = state == RD_RESP ? axi4_r_slv.pkt.data : '0;
    assign bti_rsp_mst.pkt.ok = state == RD_RESP ?
        (axi4_r_slv.pkt.resp == AXI4_R_RESP_OKAY) :
        (axi4_b_slv.pkt.resp == AXI4_B_RESP_OKAY);
    assign axi4_r_slv.rdy = state == RD_RESP && bti_rsp_mst.rdy;
    assign axi4_b_slv.rdy = state == WR_RESP && bti_rsp_mst.rdy;
endmodule
