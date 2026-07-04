`include "spec/core/isa.svh"

module sim_top(
    input logic clk,
    input logic rst_n
);
    logic uart_tx;
    logic uart_rx;
    logic uart_rx_ch_vld;
    logic [7:0] uart_rx_ch;
    logic itf_dump_en;
    logic progress_en;
    int unsigned itf_dump_count;
    int unsigned itf_dump_cycle;

    soc u_soc(
        .clk     (clk),
        .rst_n   (rst_n),
        .uart_tx (uart_tx),
        .uart_rx (uart_rx)
    );

    uart_rx u_uart_rx(
        .clk     (clk),
        .rst_n   (rst_n),
        .bc      (16'd9),
        .ch_vld  (uart_rx_ch_vld),
        .ch      (uart_rx_ch),
        .rx      (uart_tx)
    );

    always @(negedge clk) begin
        if (uart_rx_ch_vld) begin
            if (uart_rx_ch != 8'h10)
                $write("%c", uart_rx_ch);
            else
                $finish;
        end
    end

    always @(posedge clk) begin
        if (progress_en && rst_n) begin
            itf_dump_cycle <= itf_dump_cycle + 1;
            if (itf_dump_cycle % 100000 == 0)
                $display("PROGRESS cycle=%0d state=%0d pc=%08x req_pc=%08x sp=%08x",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.u_ifu.state,
                    u_soc.u_rv32g.u_hart.u_ifu.pc,
                    u_soc.u_rv32g.u_hart.u_ifu.req_pc,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[2]);
            if (itf_dump_cycle >= 5000000)
                $finish;
        end
        if (itf_dump_en && rst_n) begin
            itf_dump_cycle <= itf_dump_cycle + 1;
            if (itf_dump_cycle < 20 || itf_dump_cycle % 1000 == 0)
                $display("ITF STATUS cycle=%0d i_req=%0b/%0b i_ar=%0b/%0b d_req=%0b/%0b",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.rdy,
                    u_soc.u_rv32g.hart_i_ar.vld,
                    u_soc.u_rv32g.hart_i_ar.rdy,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.rdy);
            if (u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.rdy) begin
                $display("ITF I_REQ addr=%08x", u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.pkt.addr);
                itf_dump_count <= itf_dump_count + 1;
            end
            if (u_soc.u_rv32g.u_hart.hbi_i_bti_rsp_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_i_bti_rsp_if.rdy)
                $display("ITF I_RSP data=%08x", u_soc.u_rv32g.u_hart.hbi_i_bti_rsp_if.pkt.data);
            if (u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.rdy) begin
                $display("ITF D_REQ cmd=%0d addr=%08x data=%08x strb=%x",
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.cmd,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.addr,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.data,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.strobe);
                itf_dump_count <= itf_dump_count + 1;
            end
            if (u_soc.u_rv32g.u_hart.hbi_d_bti_rsp_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_d_bti_rsp_if.rdy)
                $display("ITF D_RSP data=%08x", u_soc.u_rv32g.u_hart.hbi_d_bti_rsp_if.pkt.data);
            if (itf_dump_count >= 500 || itf_dump_cycle >= 10000) begin
                $display("ITF dump limit reached");
                $finish;
            end
        end
    end

    initial begin
        string path;
        itf_dump_en = $test$plusargs("itf_dump");
        progress_en = $test$plusargs("progress");
        itf_dump_count = 0;
        itf_dump_cycle = 0;
        if ($value$plusargs("program=%s", path)) begin
            $readmemh(path, u_soc.u_flash.u_rom.mem);
        end
        if ($test$plusargs("vcd")) begin
            $dumpfile("sim_top.vcd");
            $dumpvars(0, sim_top);
        end
    end
endmodule
