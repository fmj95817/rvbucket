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
        S_IDLE,
        S_RD_SEND,
        S_RD_RESP,
        S_WR_SEND,
        S_WR_RESP,
        S_DONE
    } state_t;

    state_t state;
    logic [15:0] req_trans_id;
    logic [31:0] req_addr;
    bti_req_cmd_t req_cmd;
    bti_req_size_t req_size;
    logic [31:0] req_data;
    logic [3:0] req_strobe;
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic aw_done;
    logic w_done;

    wire aw_hsk = axi4_aw_mst.vld && axi4_aw_mst.rdy;
    wire w_hsk = axi4_w_mst.vld && axi4_w_mst.rdy;
    wire b_hsk = axi4_b_slv.vld && axi4_b_slv.rdy;
    wire ar_hsk = axi4_ar_mst.vld && axi4_ar_mst.rdy;
    wire r_hsk = axi4_r_slv.vld && axi4_r_slv.rdy;
    wire req_is_write = req_cmd == BTI_REQ_CMD_WRITE;

    function automatic logic req_aligned(
        input logic [31:0] addr,
        input bti_req_size_t size
    );
        unique case (size)
        BTI_REQ_SIZE_B1: req_aligned = 1'b1;
        BTI_REQ_SIZE_B2: req_aligned = addr[0] == 1'b0;
        BTI_REQ_SIZE_B4: req_aligned = addr[1:0] == 2'b00;
        default: req_aligned = 1'b0;
        endcase
    endfunction

    function automatic logic axi_resp_ok(
        input logic [1:0] resp
    );
        axi_resp_ok = resp == 2'b00;
    endfunction

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && bti_req_slv.vld && bti_req_slv.rdy) begin
            assert (req_aligned(bti_req_slv.pkt.addr, bti_req_slv.pkt.size))
                else $fatal(1, "bti2axi requires aligned BTI request");
        end
    end
`endif

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE;
            req_trans_id <= '0;
            req_addr <= '0;
            req_cmd <= BTI_REQ_CMD_READ;
            req_size <= BTI_REQ_SIZE_B1;
            req_data <= '0;
            req_strobe <= '0;
            rsp_data <= '0;
            rsp_ok <= 1'b1;
            aw_done <= 1'b0;
            w_done <= 1'b0;
        end else begin
            case (state)
            S_IDLE: begin
                if (bti_req_slv.vld && bti_req_slv.rdy) begin
                    req_trans_id <= bti_req_slv.pkt.trans_id;
                    req_addr <= bti_req_slv.pkt.addr;
                    req_cmd <= bti_req_slv.pkt.cmd;
                    req_size <= bti_req_slv.pkt.size;
                    req_data <= bti_req_slv.pkt.data;
                    req_strobe <= bti_req_slv.pkt.strobe;
                    rsp_data <= '0;
                    rsp_ok <= 1'b1;
                    aw_done <= 1'b0;
                    w_done <= 1'b0;
                    state <= bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE ?
                        S_WR_SEND : S_RD_SEND;
                end
            end
            S_RD_SEND: begin
                if (ar_hsk)
                    state <= S_RD_RESP;
            end
            S_RD_RESP: begin
                if (r_hsk) begin
                    rsp_data <= axi4_r_slv.pkt.data;
                    rsp_ok <= axi_resp_ok(axi4_r_slv.pkt.resp);
                    state <= S_DONE;
                end
            end
            S_WR_SEND: begin
                if (aw_hsk)
                    aw_done <= 1'b1;
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_done || aw_hsk) && (w_done || w_hsk))
                    state <= S_WR_RESP;
            end
            S_WR_RESP: begin
                if (b_hsk) begin
                    rsp_ok <= axi_resp_ok(axi4_b_slv.pkt.resp);
                    state <= S_DONE;
                end
            end
            S_DONE: begin
                if (bti_rsp_mst.vld && bti_rsp_mst.rdy)
                    state <= S_IDLE;
            end
            default: begin
                state <= S_IDLE;
            end
            endcase
        end
    end

    assign bti_req_slv.rdy = state == S_IDLE;

    assign axi4_ar_mst.vld = state == S_RD_SEND;
    assign axi4_ar_mst.pkt.id = '0;
    assign axi4_ar_mst.pkt.addr = req_addr;
    assign axi4_ar_mst.pkt.len = '0;
    assign axi4_ar_mst.pkt.size = axi4_ar_size_t'(req_size);
    assign axi4_ar_mst.pkt.burst = AXI4_AR_BURST_INCR;
    assign axi4_ar_mst.pkt.lock = 1'b0;
    assign axi4_ar_mst.pkt.cache = 4'h0;
    assign axi4_ar_mst.pkt.prot = 3'h0;
    assign axi4_ar_mst.pkt.qos = 4'h0;
    assign axi4_ar_mst.pkt.user = '0;

    assign axi4_aw_mst.vld = state == S_WR_SEND && !aw_done;
    assign axi4_aw_mst.pkt.id = '0;
    assign axi4_aw_mst.pkt.addr = req_addr;
    assign axi4_aw_mst.pkt.len = '0;
    assign axi4_aw_mst.pkt.size = axi4_aw_size_t'(req_size);
    assign axi4_aw_mst.pkt.burst = AXI4_AW_BURST_INCR;
    assign axi4_aw_mst.pkt.lock = 1'b0;
    assign axi4_aw_mst.pkt.cache = 4'h0;
    assign axi4_aw_mst.pkt.prot = 3'h0;
    assign axi4_aw_mst.pkt.qos = 4'h0;
    assign axi4_aw_mst.pkt.user = '0;

    assign axi4_w_mst.vld = state == S_WR_SEND && !w_done;
    assign axi4_w_mst.pkt.data = req_data;
    assign axi4_w_mst.pkt.strb = req_strobe;
    assign axi4_w_mst.pkt.last = 1'b1;

    assign axi4_r_slv.rdy = state == S_RD_RESP;
    assign axi4_b_slv.rdy = state == S_WR_RESP;

    assign bti_rsp_mst.vld = state == S_DONE;
    assign bti_rsp_mst.pkt.trans_id = req_trans_id;
    assign bti_rsp_mst.pkt.data = req_is_write ? 32'b0 : rsp_data;
    assign bti_rsp_mst.pkt.ok = rsp_ok;
endmodule
