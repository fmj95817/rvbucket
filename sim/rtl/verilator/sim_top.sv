`include "isa.svh"

module sim_top(
    input logic clk,
    input logic rst_n
);
    logic uart_tx;
    logic uart_rx;
    logic uart_rx_ch_vld;
    logic [7:0] uart_rx_ch;

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

    always @(posedge clk) begin
        if (uart_rx_ch_vld) begin
            if (uart_rx_ch == 8'h10)
                $finish;
            else
                $write("%c", uart_rx_ch);
        end
    end

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_soc.u_flash.u_rom.mem);
        end
        $dumpfile("sim_top.vcd");
        $dumpvars(0, sim_top);
    end
endmodule