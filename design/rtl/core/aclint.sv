module aclint(
    input logic clk,
    input logic rst_n,
    bti_req_if_t.slv bti_req_slv,
    bti_rsp_if_t.mst bti_rsp_mst,
    core_timer_if_t.mst core_timer_mst,
    core_m_irq_if_t.mst core_m_irq_mst
);
    logic [63:0] mtime;
    logic [63:0] mtimecmp;
    logic [11:0] tick_count;
    logic pending;
    logic [15:0] rsp_id;
    logic [31:0] rsp_data;
    wire req_hsk = bti_req_slv.vld && bti_req_slv.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            mtime <= 0;
            mtimecmp <= 64'hffffffffffffffff;
            tick_count <= 0;
            pending <= 0;
            rsp_id <= 0;
            rsp_data <= 0;
        end else begin
            tick_count <= tick_count + 1'b1;
            if (tick_count == 12'hfff) begin
                tick_count <= 0;
                mtime <= mtime + 1'b1;
            end
            if (req_hsk) begin
                pending <= 1'b1;
                rsp_id <= bti_req_slv.pkt.trans_id;
                case (bti_req_slv.pkt.addr)
                    32'h31000000: begin rsp_data <= mtime[31:0]; if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE) mtime[31:0] <= bti_req_slv.pkt.data; end
                    32'h31000004: begin rsp_data <= mtime[63:32]; if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE) mtime[63:32] <= bti_req_slv.pkt.data; end
                    32'h31010000: begin rsp_data <= mtimecmp[31:0]; if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE) mtimecmp[31:0] <= bti_req_slv.pkt.data; end
                    32'h31010004: begin rsp_data <= mtimecmp[63:32]; if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE) mtimecmp[63:32] <= bti_req_slv.pkt.data; end
                    default: rsp_data <= 0;
                endcase
            end else if (bti_rsp_mst.vld && bti_rsp_mst.rdy) begin
                pending <= 1'b0;
            end
        end
    end

    assign bti_req_slv.rdy = !pending;
    assign bti_rsp_mst.vld = pending;
    assign bti_rsp_mst.pkt.trans_id = rsp_id;
    assign bti_rsp_mst.pkt.data = rsp_data;
    assign bti_rsp_mst.pkt.ok = 1'b1;
    assign core_timer_mst.pkt.mtime = mtime;
    assign core_m_irq_mst.pkt.mtimer = mtime >= mtimecmp;
    assign core_m_irq_mst.pkt.msw = 1'b0;
endmodule
