module boot_rom(
    input logic         clk,
    input logic         cs,
    input logic  [3:0]  addr,
    output logic [31:0] data
);
    tri [31:0] mem[0:13];
    assign mem[0] = 32'h800000b7;
    assign mem[1] = 32'h10000137;
    assign mem[2] = 32'h0000a203;
    assign mem[3] = 32'h00408093;
    assign mem[4] = 32'h00000193;
    assign mem[5] = 32'h0041de63;
    assign mem[6] = 32'h003082b3;
    assign mem[7] = 32'h0002a303;
    assign mem[8] = 32'h003102b3;
    assign mem[9] = 32'h0062a023;
    assign mem[10] = 32'h00418193;
    assign mem[11] = 32'hfe9ff06f;
    assign mem[12] = 32'h100003b7;
    assign mem[13] = 32'h000380e7;

    always_ff @(posedge clk) begin
        if (cs)
            data <= mem[addr];
    end
endmodule
