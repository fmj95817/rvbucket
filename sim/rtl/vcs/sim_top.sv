module sim_top;
    localparam AW = 32;
    localparam DW = 32;

    tri clk, rst_n;

    tri   [AW-1:0]  addr;
    tri   [DW-1:0]  rdata;
    tri   [DW-1:0]  wdata;
    tri             wen;

    clk_rst u_clk_rst(.*);
    sram u_sram(.*, .addr(addr[9:0]));
    rv32i u_rv32i(.*);
endmodule