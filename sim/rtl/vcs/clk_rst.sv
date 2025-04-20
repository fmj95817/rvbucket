`timescale 1ns/1ps

module clk_rst(
    output logic clk,
    output logic rst_n
);
    always #5 clk = ~clk;

    initial begin
        clk = 1'b0;
        rst_n = 1'b1;

        #1 rst_n = 1'b0;
        #15 rst_n = 1'b1;
    end
endmodule