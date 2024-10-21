module sim_top;
    localparam AW = 32;
    localparam DW = 32;
    localparam SRAM_AW = 15;

    tri clk, rst_n;

    sram_if_t sram_rw();

    clk_rst u_clk_rst(.*);
    sram u_sram(.*);
    rv32i u_rv32i(.*);
endmodule