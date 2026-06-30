module boot_rom(
    input logic         clk,
    input logic         cs,
    input logic  [6:0]  addr,
    output logic [31:0] data
);
    tri [31:0] mem[0:83];
    assign mem[0] = 32'h800000b7;
    assign mem[1] = 32'h10000137;
    assign mem[2] = 32'h200001b7;
    assign mem[3] = 32'h0000af83;
    assign mem[4] = 32'h00100f13;
    assign mem[5] = 32'h03ff6663;
    assign mem[6] = 32'h0040a203;
    assign mem[7] = 32'h0080a283;
    assign mem[8] = 32'h00c0a503;
    assign mem[9] = 32'h0100a583;
    assign mem[10] = 32'h0140a703;
    assign mem[11] = 32'h0180a603;
    assign mem[12] = 32'h01c0a683;
    assign mem[13] = 32'h0200a783;
    assign mem[14] = 32'h02808093;
    assign mem[15] = 32'h02c0006f;
    assign mem[16] = 32'h0000a203;
    assign mem[17] = 32'h0040a283;
    assign mem[18] = 32'h00000f93;
    assign mem[19] = 32'h00000513;
    assign mem[20] = 32'h00000593;
    assign mem[21] = 32'h00000613;
    assign mem[22] = 32'h00000693;
    assign mem[23] = 32'h00000713;
    assign mem[24] = 32'h00000793;
    assign mem[25] = 32'h00808093;
    assign mem[26] = 32'h00000413;
    assign mem[27] = 32'h00445e63;
    assign mem[28] = 32'h00808333;
    assign mem[29] = 32'h00032383;
    assign mem[30] = 32'h00810333;
    assign mem[31] = 32'h00732023;
    assign mem[32] = 32'h00440413;
    assign mem[33] = 32'hfe9ff06f;
    assign mem[34] = 32'h00000413;
    assign mem[35] = 32'h00320313;
    assign mem[36] = 32'hffc37313;
    assign mem[37] = 32'h006080b3;
    assign mem[38] = 32'h00545e63;
    assign mem[39] = 32'h00808333;
    assign mem[40] = 32'h00032383;
    assign mem[41] = 32'h00818333;
    assign mem[42] = 32'h00732023;
    assign mem[43] = 32'h00440413;
    assign mem[44] = 32'hfe9ff06f;
    assign mem[45] = 32'h00328313;
    assign mem[46] = 32'hffc37313;
    assign mem[47] = 32'h006080b3;
    assign mem[48] = 32'h060f8e63;
    assign mem[49] = 32'h00000413;
    assign mem[50] = 32'h00a45e63;
    assign mem[51] = 32'h00808333;
    assign mem[52] = 32'h00032383;
    assign mem[53] = 32'h00860333;
    assign mem[54] = 32'h00732023;
    assign mem[55] = 32'h00440413;
    assign mem[56] = 32'hfe9ff06f;
    assign mem[57] = 32'h00350313;
    assign mem[58] = 32'hffc37313;
    assign mem[59] = 32'h006080b3;
    assign mem[60] = 32'h00000413;
    assign mem[61] = 32'h00b45e63;
    assign mem[62] = 32'h00808333;
    assign mem[63] = 32'h00032383;
    assign mem[64] = 32'h00868333;
    assign mem[65] = 32'h00732023;
    assign mem[66] = 32'h00440413;
    assign mem[67] = 32'hfe9ff06f;
    assign mem[68] = 32'h00358313;
    assign mem[69] = 32'hffc37313;
    assign mem[70] = 32'h006080b3;
    assign mem[71] = 32'h00000413;
    assign mem[72] = 32'h00e45e63;
    assign mem[73] = 32'h00808333;
    assign mem[74] = 32'h00032383;
    assign mem[75] = 32'h00878333;
    assign mem[76] = 32'h00732023;
    assign mem[77] = 32'h00440413;
    assign mem[78] = 32'hfe9ff06f;
    assign mem[79] = 32'h000f8663;
    assign mem[80] = 32'h00000513;
    assign mem[81] = 32'h000785b3;
    assign mem[82] = 32'h100004b7;
    assign mem[83] = 32'h000480e7;

    always_ff @(posedge clk) begin
        if (cs)
            data <= mem[addr];
    end
endmodule
