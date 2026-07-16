`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"
`include "itf/bti_req_if.svh"

module axi2bti_tb;
    localparam int REQ_NUM = 21;

    logic clk;
    logic rst_n;

    axi4_aw_if_t axi_aw();
    axi4_w_if_t axi_w();
    axi4_b_if_t axi_b();
    axi4_ar_if_t axi_ar();
    axi4_r_if_t axi_r();
    bti_req_if_t bti_req();
    bti_rsp_if_t bti_rsp();

    logic [REQ_NUM-1:0] seen;
    int unsigned rsp_num;
    int unsigned exp_rsp_idx;

    axi2bti #(
        .STG_FIFO_DEPTH (4),
        .OST_DEPTH      (8)
    ) u_dut(
        .clk         (clk),
        .rst_n       (rst_n),
        .axi4_aw_slv (axi_aw),
        .axi4_w_slv  (axi_w),
        .axi4_b_mst  (axi_b),
        .axi4_ar_slv (axi_ar),
        .axi4_r_mst  (axi_r),
        .bti_req_mst (bti_req),
        .bti_rsp_slv (bti_rsp)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        #200000;
        $display("FAIL: axi2bti_tb watchdog timeout rsp_num=%0d seen=%b",
            rsp_num, seen);
        $finish;
    end

    task automatic tick(input int unsigned n = 1);
        repeat (n) @(posedge clk);
    endtask

    task automatic send_ar(input logic [7:0] id, input logic [31:0] addr);
        begin
            axi_ar.pkt.id = id;
            axi_ar.pkt.addr = addr;
            axi_ar.pkt.len = 8'd0;
            axi_ar.pkt.size = AXI4_AR_SIZE_B4;
            axi_ar.pkt.burst = AXI4_AR_BURST_INCR;
            axi_ar.pkt.lock = 1'b0;
            axi_ar.pkt.cache = 4'h0;
            axi_ar.pkt.prot = 3'h0;
            axi_ar.pkt.qos = 4'h0;
            axi_ar.pkt.user = 32'h0;
            axi_ar.vld = 1'b1;
            while (!axi_ar.rdy)
                tick();
            tick();
            axi_ar.vld = 1'b0;
        end
    endtask

    initial begin
        rst_n = 1'b0;
        axi_aw.vld = 1'b0;
        axi_w.vld = 1'b0;
        axi_b.rdy = 1'b1;
        axi_ar.vld = 1'b0;
        axi_r.rdy = 1'b1;
        bti_req.rdy = 1'b0;
        bti_rsp.vld = 1'b0;
        bti_rsp.pkt.trans_id = '0;
        bti_rsp.pkt.data = '0;
        bti_rsp.pkt.ok = 1'b1;

        tick(4);
        rst_n = 1'b1;
        tick(2);

        fork
            begin
                for (int unsigned i = 0; i < REQ_NUM; i++) begin
                    send_ar(8'(i % 8), 32'(i * 4));
                end
            end
            begin
                wait (rsp_num == REQ_NUM);
            end
        join_any

        wait (rsp_num == REQ_NUM);
        if (seen != {REQ_NUM{1'b1}}) begin
            $display("FAIL: missing responses seen=%b", seen);
            $finish;
        end

        $display("PASS: axi2bti_tb");
        $finish;
    end

    always_comb begin
        bti_req.rdy = !(bti_rsp.vld && !bti_rsp.rdy);
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            bti_rsp.vld <= 1'b0;
            bti_rsp.pkt.trans_id <= '0;
            bti_rsp.pkt.data <= '0;
            bti_rsp.pkt.ok <= 1'b1;
        end else begin
            if (bti_rsp.vld && bti_rsp.rdy)
                bti_rsp.vld <= 1'b0;

            if (bti_req.vld && bti_req.rdy) begin
                bti_rsp.vld <= 1'b1;
                bti_rsp.pkt.trans_id <= bti_req.pkt.trans_id;
                bti_rsp.pkt.data <= 32'habc00000 |
                    {24'h0, bti_req.pkt.addr[9:2]};
                bti_rsp.pkt.ok <= 1'b1;
            end
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            seen <= '0;
            rsp_num <= 0;
            exp_rsp_idx <= 0;
        end else if (axi_r.vld && axi_r.rdy) begin
            int unsigned idx;
            idx = int'(axi_r.pkt.data[7:0]);
            if (idx >= REQ_NUM) begin
                $display("FAIL: r data index out of range data=%08x",
                    axi_r.pkt.data);
                $finish;
            end
            if (seen[idx]) begin
                $display("FAIL: duplicate response idx=%0d", idx);
                $finish;
            end
            if (idx != exp_rsp_idx) begin
                $display("FAIL: response order mismatch exp=%0d got=%0d",
                    exp_rsp_idx, idx);
                $finish;
            end
            if (axi_r.pkt.id !== 8'(idx % 8)) begin
                $display("FAIL: r id mismatch idx=%0d exp=%02x got=%02x",
                    idx, 8'(idx % 8), axi_r.pkt.id);
                $finish;
            end
            if (axi_r.pkt.resp !== AXI4_R_RESP_OKAY || !axi_r.pkt.last) begin
                $display("FAIL: r resp/last mismatch resp=%0d last=%0b",
                    axi_r.pkt.resp, axi_r.pkt.last);
                $finish;
            end
            seen[idx] <= 1'b1;
            rsp_num <= rsp_num + 1;
            exp_rsp_idx <= exp_rsp_idx + 1;
        end
    end
endmodule
