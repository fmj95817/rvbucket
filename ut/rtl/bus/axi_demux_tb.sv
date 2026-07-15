`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi_demux_tb;
    localparam GST_NUM = 5;

    logic clk;
    logic rst_n;

    axi4_aw_if_t host_aw();
    axi4_w_if_t host_w();
    axi4_b_if_t host_b();
    axi4_ar_if_t host_ar();
    axi4_r_if_t host_r();
    axi4_aw_if_t gst_aw[GST_NUM]();
    axi4_w_if_t gst_w[GST_NUM]();
    axi4_b_if_t gst_b[GST_NUM]();
    axi4_ar_if_t gst_ar[GST_NUM]();
    axi4_r_if_t gst_r[GST_NUM]();

    logic [7:0] gst1_aw_id;
    logic gst1_aw_seen;
    logic gst1_b_pending;

    axi_demux #(
        .GST_NUM (GST_NUM),
        .GST_BASE('{32'h00001000, 32'h00002000, 32'h00003000,
            32'h00004000, 32'h00005000}),
        .GST_SIZE('{32'h00001000, 32'h00001000, 32'h00001000,
            32'h00001000, 32'h00001000}),
        .STG_FIFO_DEPTH(4),
        .OST_DEPTH(4)
    ) u_dut(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (host_aw),
        .host_axi4_w_slv  (host_w),
        .host_axi4_b_mst  (host_b),
        .host_axi4_ar_slv (host_ar),
        .host_axi4_r_mst  (host_r),
        .gst_axi4_aw_msts (gst_aw),
        .gst_axi4_w_msts  (gst_w),
        .gst_axi4_b_slvs  (gst_b),
        .gst_axi4_ar_msts (gst_ar),
        .gst_axi4_r_slvs  (gst_r)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    task automatic tick(input int unsigned n = 1);
        repeat (n) @(posedge clk);
    endtask

    task automatic send_aw(input logic [7:0] id, input logic [31:0] addr);
        begin
            host_aw.pkt.id = id;
            host_aw.pkt.addr = addr;
            host_aw.pkt.len = 8'd0;
            host_aw.pkt.size = AXI4_AW_SIZE_B4;
            host_aw.pkt.burst = AXI4_AW_BURST_INCR;
            host_aw.pkt.lock = 1'b0;
            host_aw.pkt.cache = 4'h0;
            host_aw.pkt.prot = 3'h0;
            host_aw.pkt.qos = 4'h0;
            host_aw.pkt.user = 32'h0;
            host_aw.vld = 1'b1;
            do tick(); while (!host_aw.rdy);
            host_aw.vld = 1'b0;
        end
    endtask

    task automatic send_w(input logic [31:0] data);
        begin
            host_w.pkt.data = data;
            host_w.pkt.strb = 4'hf;
            host_w.pkt.last = 1'b1;
            host_w.vld = 1'b1;
            do tick(); while (!host_w.rdy);
            host_w.vld = 1'b0;
        end
    endtask

    task automatic wait_b(input logic [7:0] exp_id);
        begin
            host_b.rdy = 1'b1;
            for (int unsigned i = 0; i < 100; i++) begin
                tick();
                if (host_b.vld) begin
                    if (host_b.pkt.id !== exp_id) begin
                        $display("FAIL: b id exp=%02x got=%02x",
                            exp_id, host_b.pkt.id);
                        $finish;
                    end
                    if (host_b.pkt.resp !== AXI4_B_RESP_OKAY) begin
                        $display("FAIL: b resp %0d", host_b.pkt.resp);
                        $finish;
                    end
                    host_b.rdy = 1'b0;
                    return;
                end
            end
            $display("FAIL: timeout waiting b");
            $finish;
        end
    endtask

    initial begin
        rst_n = 1'b0;
        host_aw.vld = 1'b0;
        host_w.vld = 1'b0;
        host_b.rdy = 1'b0;
        host_ar.vld = 1'b0;
        host_r.rdy = 1'b0;
        gst_aw[0].rdy = 1'b1;
        gst_aw[1].rdy = 1'b1;
        gst_aw[2].rdy = 1'b1;
        gst_aw[3].rdy = 1'b1;
        gst_aw[4].rdy = 1'b1;
        gst_w[0].rdy = 1'b1;
        gst_w[1].rdy = 1'b1;
        gst_w[2].rdy = 1'b1;
        gst_w[3].rdy = 1'b1;
        gst_w[4].rdy = 1'b1;
        gst_ar[0].rdy = 1'b1;
        gst_ar[1].rdy = 1'b1;
        gst_ar[2].rdy = 1'b1;
        gst_ar[3].rdy = 1'b1;
        gst_ar[4].rdy = 1'b1;

        gst_b[0].vld = 1'b0;
        gst_b[0].pkt.id = '0;
        gst_b[0].pkt.resp = AXI4_B_RESP_OKAY;
        gst_b[1].vld = 1'b0;
        gst_b[1].pkt.id = '0;
        gst_b[1].pkt.resp = AXI4_B_RESP_OKAY;
        gst_b[2].vld = 1'b0;
        gst_b[2].pkt.id = '0;
        gst_b[2].pkt.resp = AXI4_B_RESP_OKAY;
        gst_b[3].vld = 1'b0;
        gst_b[3].pkt.id = '0;
        gst_b[3].pkt.resp = AXI4_B_RESP_OKAY;
        gst_b[4].vld = 1'b0;
        gst_b[4].pkt.id = '0;
        gst_b[4].pkt.resp = AXI4_B_RESP_OKAY;

        gst_r[0].vld = 1'b0;
        gst_r[0].pkt.id = '0;
        gst_r[0].pkt.data = '0;
        gst_r[0].pkt.resp = AXI4_R_RESP_OKAY;
        gst_r[0].pkt.last = 1'b1;
        gst_r[1].vld = 1'b0;
        gst_r[1].pkt.id = '0;
        gst_r[1].pkt.data = '0;
        gst_r[1].pkt.resp = AXI4_R_RESP_OKAY;
        gst_r[1].pkt.last = 1'b1;
        gst_r[2].vld = 1'b0;
        gst_r[2].pkt.id = '0;
        gst_r[2].pkt.data = '0;
        gst_r[2].pkt.resp = AXI4_R_RESP_OKAY;
        gst_r[2].pkt.last = 1'b1;
        gst_r[3].vld = 1'b0;
        gst_r[3].pkt.id = '0;
        gst_r[3].pkt.data = '0;
        gst_r[3].pkt.resp = AXI4_R_RESP_OKAY;
        gst_r[3].pkt.last = 1'b1;
        gst_r[4].vld = 1'b0;
        gst_r[4].pkt.id = '0;
        gst_r[4].pkt.data = '0;
        gst_r[4].pkt.resp = AXI4_R_RESP_OKAY;
        gst_r[4].pkt.last = 1'b1;

        tick(4);
        rst_n = 1'b1;
        tick(2);

        send_aw(8'h35, 32'h00002010);
        send_w(32'h11223344);
        wait_b(8'h35);

        $display("PASS: axi_demux_tb");
        $finish;
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            gst1_aw_id <= '0;
            gst1_aw_seen <= 1'b0;
            gst1_b_pending <= 1'b0;
            gst_b[1].vld <= 1'b0;
            gst_b[1].pkt.id <= '0;
            gst_b[1].pkt.resp <= AXI4_B_RESP_OKAY;
        end else begin
            if (gst_aw[1].vld && gst_aw[1].rdy) begin
                gst1_aw_id <= gst_aw[1].pkt.id;
                gst1_aw_seen <= 1'b1;
            end

            if (gst1_aw_seen && gst_w[1].vld && gst_w[1].rdy &&
                gst_w[1].pkt.last) begin
                gst1_b_pending <= 1'b1;
                gst_b[1].vld <= 1'b1;
                gst_b[1].pkt.id <= gst1_aw_id;
                gst_b[1].pkt.resp <= AXI4_B_RESP_OKAY;
            end

            if (gst_b[1].vld && gst_b[1].rdy) begin
                gst1_aw_seen <= 1'b0;
                gst1_b_pending <= 1'b0;
                gst_b[1].vld <= 1'b0;
            end
        end
    end

    wire unused = gst1_b_pending;
endmodule
