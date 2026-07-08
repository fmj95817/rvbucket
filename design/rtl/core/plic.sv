`include "itf/bti_req_if.svh"

module plic(
    input logic clk,
    input logic rst_n,
    bti_req_if_t.slv bti_req_slv,
    bti_rsp_if_t.mst bti_rsp_mst,
    ext_irq_if_t.slv uart_irq_slv,
    ext_irq_if_t.mst core_irq_mst
);
    localparam SOURCE_NUM = 33;
    localparam BASE = 32'h31100000;
    logic [2:0] priorities[0:SOURCE_NUM-1];
    logic [SOURCE_NUM-1:0] pending;
    logic [SOURCE_NUM-1:0] claimed;
    logic [SOURCE_NUM-1:0] enable;
    logic [2:0] threshold;
    logic rsp_pending;
    logic [15:0] rsp_id;
    logic [31:0] rsp_data;
    logic rsp_ok;

    logic [5:0] best_source;
    logic [2:0] best_priority;
    always_comb begin
        best_source = 0;
        best_priority = threshold;
        for (int source = 1; source < SOURCE_NUM; source++) begin
            if (pending[source] && !claimed[source] && enable[source] &&
                priorities[source] > best_priority) begin
                best_source = source[5:0];
                best_priority = priorities[source];
            end
        end
    end

    wire req_hsk = bti_req_slv.vld && bti_req_slv.rdy;
    wire rsp_hsk = bti_rsp_mst.vld && bti_rsp_mst.rdy;
    wire [31:0] offset = bti_req_slv.pkt.addr - BASE;
    integer i;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < SOURCE_NUM; i++) priorities[i] <= 0;
            pending <= 0;
            claimed <= 0;
            enable <= 0;
            threshold <= 0;
            rsp_pending <= 0;
            rsp_id <= 0;
            rsp_data <= 0;
            rsp_ok <= 1'b1;
        end else begin
            if (uart_irq_slv.pkt.irq && !claimed[1]) pending[1] <= 1'b1;
            if (rsp_hsk) rsp_pending <= 1'b0;
            if (req_hsk) begin
                rsp_pending <= 1'b1;
                rsp_id <= bti_req_slv.pkt.trans_id;
                rsp_data <= 0;
                rsp_ok <= 1'b1;
                if (offset < SOURCE_NUM * 4) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE && offset[31:2] != 0)
                        priorities[offset[7:2]] <= bti_req_slv.pkt.data[2:0];
                    else if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_READ)
                        rsp_data <= {29'b0, priorities[offset[7:2]]};
                end else if (offset == 32'h00001000) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_READ)
                        rsp_data <= pending[31:0] & 32'hfffffffe;
                end else if (offset == 32'h00001004) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_READ)
                        rsp_data <= {31'b0, pending[32]};
                end else if (offset == 32'h00002000) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE)
                        enable[31:0] <= bti_req_slv.pkt.data & 32'hfffffffe;
                    else rsp_data <= enable[31:0];
                end else if (offset == 32'h00002004) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE)
                        enable[32] <= bti_req_slv.pkt.data[0];
                    else rsp_data <= {31'b0, enable[32]};
                end else if (offset == 32'h00200000) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE)
                        threshold <= bti_req_slv.pkt.data[2:0];
                    else rsp_data <= {29'b0, threshold};
                end else if (offset == 32'h00200004) begin
                    if (bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE) begin
                        if (bti_req_slv.pkt.data > 0 && bti_req_slv.pkt.data < SOURCE_NUM)
                            claimed[bti_req_slv.pkt.data[5:0]] <= 1'b0;
                    end else begin
                        rsp_data <= {26'b0, best_source};
                        if (best_source != 0) begin
                            pending[best_source] <= 1'b0;
                            claimed[best_source] <= 1'b1;
                        end
                    end
                end else begin
                    rsp_ok <= 1'b0;
                end
            end
        end
    end

    assign bti_req_slv.rdy = !rsp_pending;
    assign bti_rsp_mst.vld = rsp_pending;
    assign bti_rsp_mst.pkt.trans_id = rsp_id;
    assign bti_rsp_mst.pkt.data = rsp_data;
    assign bti_rsp_mst.pkt.ok = rsp_ok;
    assign core_irq_mst.pkt.irq = best_source != 0;
endmodule
