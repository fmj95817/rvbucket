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
    typedef enum logic [2:0] {
        S_IDLE,
        S_RD_REQ,
        S_RD_RSP,
        S_RD_SEND,
        S_WR_WAIT_W,
        S_WR_REQ,
        S_WR_RSP,
        S_WR_SEND_B
    } state_t;

    state_t state;
    logic [7:0] ax_id;
    logic [31:0] ax_addr;
    logic [7:0] ax_len;
    logic [2:0] ax_size;
    logic [1:0] ax_burst;
    logic [7:0] beat_idx;
    logic rsp_ok;
    logic [31:0] rd_data;
    logic [31:0] wr_data;
    logic [3:0] wr_strobe;
    logic wr_last;
    logic addr_sel_rd;

    wire req_hsk = bti_req_mst.vld && bti_req_mst.rdy;
    wire rsp_hsk = bti_rsp_slv.vld && bti_rsp_slv.rdy;
    wire ar_hsk = axi4_ar_slv.vld && axi4_ar_slv.rdy;
    wire aw_hsk = axi4_aw_slv.vld && axi4_aw_slv.rdy;
    wire w_hsk = axi4_w_slv.vld && axi4_w_slv.rdy;
    wire r_hsk = axi4_r_mst.vld && axi4_r_mst.rdy;
    wire b_hsk = axi4_b_mst.vld && axi4_b_mst.rdy;

    function automatic logic [31:0] next_addr(
        input logic [31:0] addr,
        input logic [2:0] size,
        input logic [1:0] burst
    );
        unique case (burst)
        2'd0: next_addr = addr;
        default: next_addr = addr + (32'd1 << size);
        endcase
    endfunction

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE;
            ax_id <= '0;
            ax_addr <= '0;
            ax_len <= '0;
            ax_size <= '0;
            ax_burst <= '0;
            beat_idx <= '0;
            rsp_ok <= 1'b1;
            rd_data <= '0;
            wr_data <= '0;
            wr_strobe <= '0;
            wr_last <= 1'b0;
            addr_sel_rd <= 1'b0;
        end else begin
            unique case (state)
            S_IDLE: begin
                rsp_ok <= 1'b1;
                beat_idx <= '0;
                if (aw_hsk) begin
                    ax_id <= axi4_aw_slv.pkt.id;
                    ax_addr <= axi4_aw_slv.pkt.addr;
                    ax_len <= axi4_aw_slv.pkt.len;
                    ax_size <= axi4_aw_slv.pkt.size;
                    ax_burst <= axi4_aw_slv.pkt.burst;
                    addr_sel_rd <= 1'b1;
                    state <= S_WR_WAIT_W;
                end else if (ar_hsk) begin
                    ax_id <= axi4_ar_slv.pkt.id;
                    ax_addr <= axi4_ar_slv.pkt.addr;
                    ax_len <= axi4_ar_slv.pkt.len;
                    ax_size <= axi4_ar_slv.pkt.size;
                    ax_burst <= axi4_ar_slv.pkt.burst;
                    addr_sel_rd <= 1'b0;
                    state <= S_RD_REQ;
                end else if (!addr_sel_rd && !axi4_aw_slv.vld && axi4_ar_slv.vld) begin
                    addr_sel_rd <= 1'b1;
                end else if (addr_sel_rd && !axi4_ar_slv.vld && axi4_aw_slv.vld) begin
                    addr_sel_rd <= 1'b0;
                end
            end
            S_RD_REQ: begin
                if (req_hsk)
                    state <= S_RD_RSP;
            end
            S_RD_RSP: begin
                if (rsp_hsk) begin
                    rd_data <= bti_rsp_slv.pkt.data;
                    rsp_ok <= rsp_ok && bti_rsp_slv.pkt.ok;
                    state <= S_RD_SEND;
                end
            end
            S_RD_SEND: begin
                if (r_hsk) begin
                    if (beat_idx == ax_len || !rsp_ok) begin
                        state <= S_IDLE;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        ax_addr <= next_addr(ax_addr, ax_size, ax_burst);
                        state <= S_RD_REQ;
                    end
                end
            end
            S_WR_WAIT_W: begin
                if (w_hsk) begin
                    wr_data <= axi4_w_slv.pkt.data;
                    wr_strobe <= axi4_w_slv.pkt.strb;
                    wr_last <= axi4_w_slv.pkt.last || beat_idx == ax_len;
                    state <= S_WR_REQ;
                end
            end
            S_WR_REQ: begin
                if (req_hsk)
                    state <= S_WR_RSP;
            end
            S_WR_RSP: begin
                if (rsp_hsk) begin
                    rsp_ok <= rsp_ok && bti_rsp_slv.pkt.ok;
                    if (wr_last) begin
                        state <= S_WR_SEND_B;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        ax_addr <= next_addr(ax_addr, ax_size, ax_burst);
                        state <= S_WR_WAIT_W;
                    end
                end
            end
            S_WR_SEND_B: begin
                if (b_hsk)
                    state <= S_IDLE;
            end
            default: begin
                state <= S_IDLE;
            end
            endcase
        end
    end

    assign bti_req_mst.vld = state == S_RD_REQ || state == S_WR_REQ;
    assign bti_req_mst.pkt.trans_id = '0;
    assign bti_req_mst.pkt.cmd = state == S_WR_REQ ?
        BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
    assign bti_req_mst.pkt.addr = ax_addr;
    assign bti_req_mst.pkt.size = bti_req_size_t'(ax_size[1:0]);
    assign bti_req_mst.pkt.data = wr_data;
    assign bti_req_mst.pkt.strobe = wr_strobe;

    assign axi4_aw_slv.rdy = state == S_IDLE && !addr_sel_rd;
    assign axi4_w_slv.rdy = state == S_WR_WAIT_W;
    assign axi4_ar_slv.rdy = state == S_IDLE && addr_sel_rd;

    assign axi4_r_mst.vld = state == S_RD_SEND;
    assign axi4_r_mst.pkt.id = ax_id;
    assign axi4_r_mst.pkt.data = rd_data;
    assign axi4_r_mst.pkt.resp = rsp_ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR;
    assign axi4_r_mst.pkt.last = beat_idx == ax_len || !rsp_ok;

    assign axi4_b_mst.vld = state == S_WR_SEND_B;
    assign axi4_b_mst.pkt.id = ax_id;
    assign axi4_b_mst.pkt.resp = rsp_ok ? AXI4_B_RESP_OKAY : AXI4_B_RESP_SLVERR;
    assign bti_rsp_slv.rdy = state == S_RD_RSP || state == S_WR_RSP;
endmodule
