`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi2apb_tb;
    logic clk;
    logic rst_n;

    axi4_aw_if_t axi_aw();
    axi4_w_if_t axi_w();
    axi4_b_if_t axi_b();
    axi4_ar_if_t axi_ar();
    axi4_r_if_t axi_r();
    apb_req_if_t apb_req();
    apb_rsp_if_t apb_rsp();

    axi2apb u_dut(
        .clk         (clk),
        .rst_n       (rst_n),
        .axi4_aw_slv (axi_aw),
        .axi4_w_slv  (axi_w),
        .axi4_b_mst  (axi_b),
        .axi4_ar_slv (axi_ar),
        .axi4_r_mst  (axi_r),
        .apb_req_mst (apb_req),
        .apb_rsp_slv (apb_rsp)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        #5000;
        $display("FAIL: axi2apb_tb watchdog timeout");
        $finish;
    end

    task automatic tick(input int unsigned n = 1);
        repeat (n) begin
            @(posedge clk);
            #1;
        end
    endtask

    task automatic send_aw(input logic [7:0] id, input logic [31:0] addr);
        begin
            axi_aw.pkt.id = id;
            axi_aw.pkt.addr = addr;
            axi_aw.pkt.len = 8'd0;
            axi_aw.pkt.size = AXI4_AW_SIZE_B4;
            axi_aw.pkt.burst = AXI4_AW_BURST_INCR;
            axi_aw.pkt.lock = 1'b0;
            axi_aw.pkt.cache = 4'h0;
            axi_aw.pkt.prot = 3'h0;
            axi_aw.pkt.qos = 4'h0;
            axi_aw.pkt.user = 32'h0;
            axi_aw.vld = 1'b1;
            while (!axi_aw.rdy)
                tick();
            tick();
            axi_aw.vld = 1'b0;
        end
    endtask

    task automatic send_w(input logic [31:0] data, input logic [3:0] strb);
        begin
            axi_w.pkt.data = data;
            axi_w.pkt.strb = strb;
            axi_w.pkt.last = 1'b1;
            axi_w.vld = 1'b1;
            while (!axi_w.rdy)
                tick();
            tick();
            axi_w.vld = 1'b0;
        end
    endtask

    task automatic wait_b(input logic [7:0] exp_id);
        begin
            axi_b.rdy = 1'b0;
            for (int unsigned i = 0; i < 100; i++) begin
                if (axi_b.vld) begin
                    if (axi_b.pkt.id !== exp_id) begin
                        $display("FAIL: b id exp=%02x got=%02x",
                            exp_id, axi_b.pkt.id);
                        $finish;
                    end
                    if (axi_b.pkt.resp !== AXI4_B_RESP_OKAY) begin
                        $display("FAIL: b resp %0d", axi_b.pkt.resp);
                        $finish;
                    end
                    axi_b.rdy = 1'b1;
                    tick();
                    axi_b.rdy = 1'b0;
                    return;
                end
                tick();
            end
            $display("FAIL: timeout waiting b");
            $finish;
        end
    endtask

    initial begin
        rst_n = 1'b0;
        axi_aw.vld = 1'b0;
        axi_w.vld = 1'b0;
        axi_b.rdy = 1'b0;
        axi_ar.vld = 1'b0;
        axi_r.rdy = 1'b0;
        apb_rsp.pready = 1'b1;
        apb_rsp.pkt.prdata = 32'h0;
        apb_rsp.pkt.pslverr = 1'b0;

        tick(4);
        rst_n = 1'b1;
        tick(2);

        $display("INFO: AW before W");
        send_aw(8'h12, 32'h30000000);
        tick(3);
        send_w(32'h87654321, 4'hf);
        wait_b(8'h12);

        $display("INFO: W before AW");
        send_w(32'h12345678, 4'hf);
        tick(3);
        send_aw(8'h34, 32'h30000004);
        wait_b(8'h34);

        $display("PASS: axi2apb_tb");
        $finish;
    end

    always_ff @(posedge clk) begin
        if (rst_n && apb_req.psel && apb_req.penable) begin
            if (!apb_req.pkt.pwrite) begin
                $display("FAIL: unexpected APB read");
                $finish;
            end
        end
    end
endmodule
