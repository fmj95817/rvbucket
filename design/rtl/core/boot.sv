module boot_rom(
    input logic         clk,
    input logic         cs,
    input logic  [4:0]  addr,
    output logic [31:0] data
);
    tri [31:0] mem[0:24];
    assign mem[0] = 32'h800000b7;
    assign mem[1] = 32'h10000137;
    assign mem[2] = 32'h200001b7;
    assign mem[3] = 32'h0000a203;
    assign mem[4] = 32'h0040a283;
    assign mem[5] = 32'h00808093;
    assign mem[6] = 32'h00000413;
    assign mem[7] = 32'h00445e63;
    assign mem[8] = 32'h00808333;
    assign mem[9] = 32'h00032383;
    assign mem[10] = 32'h00810333;
    assign mem[11] = 32'h00732023;
    assign mem[12] = 32'h00440413;
    assign mem[13] = 32'hfe9ff06f;
    assign mem[14] = 32'h00000413;
    assign mem[15] = 32'h004080b3;
    assign mem[16] = 32'h00545e63;
    assign mem[17] = 32'h00808333;
    assign mem[18] = 32'h00032383;
    assign mem[19] = 32'h00818333;
    assign mem[20] = 32'h00732023;
    assign mem[21] = 32'h00440413;
    assign mem[22] = 32'hfe9ff06f;
    assign mem[23] = 32'h100004b7;
    assign mem[24] = 32'h000480e7;

    always_ff @(posedge clk) begin
        if (cs)
            data <= mem[addr];
    end
endmodule
