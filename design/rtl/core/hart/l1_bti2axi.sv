`include "itf/bti_req_if.svh"
`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

// Temporary bridge for the current RTL L1 pass-through implementation.
// Delete this file after RTL L1 implements cache/bypass handling itself.
module l1_bti2axi(
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
    typedef enum logic [3:0] {
        S_IDLE,
        S_RD_SEND0,
        S_RD_RESP0,
        S_RD_SEND1,
        S_RD_RESP1,
        S_WR_SEND0,
        S_WR_RESP0,
        S_WR_SEND1,
        S_WR_RESP1,
        S_DONE
    } state_t;

    state_t state;
    logic [15:0] req_trans_id;
    logic [31:0] req_addr;
    bti_req_cmd_t req_cmd;
    bti_req_size_t req_size;
    logic [31:0] req_data;
    logic [3:0] req_strobe;
    logic req_split;
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic aw_done;
    logic w_done;

    wire aw_hsk = axi4_aw_mst.vld && axi4_aw_mst.rdy;
    wire w_hsk = axi4_w_mst.vld && axi4_w_mst.rdy;
    wire b_hsk = axi4_b_slv.vld && axi4_b_slv.rdy;
    wire ar_hsk = axi4_ar_mst.vld && axi4_ar_mst.rdy;
    wire r_hsk = axi4_r_slv.vld && axi4_r_slv.rdy;
    wire [1:0] req_off = req_addr[1:0];
    wire [31:0] req_aligned_addr = {req_addr[31:2], 2'b00};
    wire req_is_write = req_cmd == BTI_REQ_CMD_WRITE;
    wire wr_second = state == S_WR_SEND1 || state == S_WR_RESP1;

    function automatic logic [2:0] req_byte_num(
        input bti_req_size_t size
    );
        unique case (size)
        BTI_REQ_SIZE_B1: req_byte_num = 3'd1;
        BTI_REQ_SIZE_B2: req_byte_num = 3'd2;
        default: req_byte_num = 3'd4;
        endcase
    endfunction

    function automatic logic req_cross_word(
        input logic [1:0] off,
        input bti_req_size_t size
    );
        req_cross_word = ({1'b0, off} + req_byte_num(size)) > 3'd4;
    endfunction

    function automatic logic [31:0] align_read_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        unique case (off)
        2'd0: align_read_data = data;
        2'd1: align_read_data = {8'b0, data[31:8]};
        2'd2: align_read_data = {16'b0, data[31:16]};
        2'd3: align_read_data = {24'b0, data[31:24]};
        default: align_read_data = 32'hx;
        endcase
    endfunction

    function automatic logic [31:0] merge_second_read_data(
        input logic [31:0] first_data,
        input logic [31:0] second_data,
        input logic [1:0] off
    );
        unique case (off)
        2'd1: merge_second_read_data = first_data | {second_data[7:0], 24'b0};
        2'd2: merge_second_read_data = first_data | {second_data[15:0], 16'b0};
        2'd3: merge_second_read_data = first_data | {second_data[23:0], 8'b0};
        default: merge_second_read_data = first_data;
        endcase
    endfunction

    function automatic logic [31:0] first_write_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        unique case (off)
        2'd0: first_write_data = data;
        2'd1: first_write_data = {data[23:0], 8'b0};
        2'd2: first_write_data = {data[15:0], 16'b0};
        2'd3: first_write_data = {data[7:0], 24'b0};
        default: first_write_data = 32'hx;
        endcase
    endfunction

    function automatic logic [3:0] first_write_strobe(
        input logic [3:0] strobe,
        input logic [1:0] off
    );
        unique case (off)
        2'd0: first_write_strobe = strobe;
        2'd1: first_write_strobe = {strobe[2:0], 1'b0};
        2'd2: first_write_strobe = {strobe[1:0], 2'b0};
        2'd3: first_write_strobe = {strobe[0], 3'b0};
        default: first_write_strobe = 4'hx;
        endcase
    endfunction

    function automatic logic [31:0] second_write_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        unique case (off)
        2'd1: second_write_data = {24'b0, data[31:24]};
        2'd2: second_write_data = {16'b0, data[31:16]};
        2'd3: second_write_data = {8'b0, data[31:8]};
        default: second_write_data = 32'b0;
        endcase
    endfunction

    function automatic logic [3:0] second_write_strobe(
        input logic [3:0] strobe,
        input logic [1:0] off
    );
        unique case (off)
        2'd1: second_write_strobe = {3'b0, strobe[3]};
        2'd2: second_write_strobe = {2'b0, strobe[3:2]};
        2'd3: second_write_strobe = {1'b0, strobe[3:1]};
        default: second_write_strobe = 4'b0;
        endcase
    endfunction

    function automatic logic axi_resp_ok(
        input logic [1:0] resp
    );
        axi_resp_ok = resp == 2'b00;
    endfunction

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE;
            req_trans_id <= '0;
            req_addr <= '0;
            req_cmd <= BTI_REQ_CMD_READ;
            req_size <= BTI_REQ_SIZE_B1;
            req_data <= '0;
            req_strobe <= '0;
            req_split <= 1'b0;
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
                    req_split <= req_cross_word(bti_req_slv.pkt.addr[1:0],
                        bti_req_slv.pkt.size);
                    rsp_data <= '0;
                    rsp_ok <= 1'b1;
                    aw_done <= 1'b0;
                    w_done <= 1'b0;
                    state <= bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE ?
                        S_WR_SEND0 : S_RD_SEND0;
                end
            end
            S_RD_SEND0: begin
                if (ar_hsk)
                    state <= S_RD_RESP0;
            end
            S_RD_RESP0: begin
                if (r_hsk) begin
                    rsp_data <= align_read_data(axi4_r_slv.pkt.data, req_off);
                    rsp_ok <= axi_resp_ok(axi4_r_slv.pkt.resp);
                    state <= req_split ? S_RD_SEND1 : S_DONE;
                end
            end
            S_RD_SEND1: begin
                if (ar_hsk)
                    state <= S_RD_RESP1;
            end
            S_RD_RESP1: begin
                if (r_hsk) begin
                    rsp_data <= merge_second_read_data(rsp_data,
                        axi4_r_slv.pkt.data, req_off);
                    rsp_ok <= rsp_ok && axi_resp_ok(axi4_r_slv.pkt.resp);
                    state <= S_DONE;
                end
            end
            S_WR_SEND0: begin
                if (aw_hsk)
                    aw_done <= 1'b1;
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_done || aw_hsk) && (w_done || w_hsk))
                    state <= S_WR_RESP0;
            end
            S_WR_RESP0: begin
                if (b_hsk) begin
                    rsp_ok <= axi_resp_ok(axi4_b_slv.pkt.resp);
                    if (req_split) begin
                        aw_done <= 1'b0;
                        w_done <= 1'b0;
                        state <= S_WR_SEND1;
                    end else begin
                        state <= S_DONE;
                    end
                end
            end
            S_WR_SEND1: begin
                if (aw_hsk)
                    aw_done <= 1'b1;
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_done || aw_hsk) && (w_done || w_hsk))
                    state <= S_WR_RESP1;
            end
            S_WR_RESP1: begin
                if (b_hsk) begin
                    rsp_ok <= rsp_ok && axi_resp_ok(axi4_b_slv.pkt.resp);
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

    assign axi4_ar_mst.vld = state == S_RD_SEND0 || state == S_RD_SEND1;
    assign axi4_ar_mst.pkt.id = '0;
    assign axi4_ar_mst.pkt.addr = state == S_RD_SEND1 ?
        (req_aligned_addr + 32'd4) : req_aligned_addr;
    assign axi4_ar_mst.pkt.len = '0;
    assign axi4_ar_mst.pkt.size = AXI4_AR_SIZE_B4;
    assign axi4_ar_mst.pkt.burst = AXI4_AR_BURST_INCR;
    assign axi4_ar_mst.pkt.lock = 1'b0;
    assign axi4_ar_mst.pkt.cache = 4'h0;
    assign axi4_ar_mst.pkt.prot = 3'h0;
    assign axi4_ar_mst.pkt.qos = 4'h0;
    assign axi4_ar_mst.pkt.user = '0;

    assign axi4_aw_mst.vld = (state == S_WR_SEND0 || state == S_WR_SEND1) &&
        !aw_done;
    assign axi4_aw_mst.pkt.id = '0;
    assign axi4_aw_mst.pkt.addr = wr_second ?
        (req_aligned_addr + 32'd4) : req_aligned_addr;
    assign axi4_aw_mst.pkt.len = '0;
    assign axi4_aw_mst.pkt.size = AXI4_AW_SIZE_B4;
    assign axi4_aw_mst.pkt.burst = AXI4_AW_BURST_INCR;
    assign axi4_aw_mst.pkt.lock = 1'b0;
    assign axi4_aw_mst.pkt.cache = 4'h0;
    assign axi4_aw_mst.pkt.prot = 3'h0;
    assign axi4_aw_mst.pkt.qos = 4'h0;
    assign axi4_aw_mst.pkt.user = '0;

    assign axi4_w_mst.vld = (state == S_WR_SEND0 || state == S_WR_SEND1) &&
        !w_done;
    assign axi4_w_mst.pkt.data = wr_second ?
        second_write_data(req_data, req_off) :
        first_write_data(req_data, req_off);
    assign axi4_w_mst.pkt.strb = wr_second ?
        second_write_strobe(req_strobe, req_off) :
        first_write_strobe(req_strobe, req_off);
    assign axi4_w_mst.pkt.last = 1'b1;

    assign axi4_r_slv.rdy = state == S_RD_RESP0 || state == S_RD_RESP1;
    assign axi4_b_slv.rdy = state == S_WR_RESP0 || state == S_WR_RESP1;

    assign bti_rsp_mst.vld = state == S_DONE;
    assign bti_rsp_mst.pkt.trans_id = req_trans_id;
    assign bti_rsp_mst.pkt.data = req_is_write ? 32'b0 : rsp_data;
    assign bti_rsp_mst.pkt.ok = rsp_ok;
endmodule
