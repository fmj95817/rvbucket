module boot_rom(
    input tri           clk,
    input tri    [3:0]  rom_addr,
    output logic [31:0] rom_data
);
    tri [31:0] data[0:13];
    assign data[0] = 32'h800000b7;
    assign data[1] = 32'h10000137;
    assign data[2] = 32'h0000a203;
    assign data[3] = 32'h00408093;
    assign data[4] = 32'h00000193;
    assign data[5] = 32'h0041de63;
    assign data[6] = 32'h003082b3;
    assign data[7] = 32'h0002a303;
    assign data[8] = 32'h003102b3;
    assign data[9] = 32'h0062a023;
    assign data[10] = 32'h00418193;
    assign data[11] = 32'hfe9ff06f;
    assign data[12] = 32'h100003b7;
    assign data[13] = 32'h000380e7;

    always_ff @(posedge clk) begin
        rom_data <= data[rom_addr];
    end
endmodule
