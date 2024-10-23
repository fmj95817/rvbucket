module sim_top;
    localparam AW = 32;
    localparam DW = 32;
    localparam SRAM_AW = 15;

    tri clk, rst_n;
    bus_trans_if_t bti();

    clk_rst u_clk_rst(.*);
    bti_sram u_bti_sram(.*);
    rv32i u_rv32i(.*);

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_bti_sram.u_sram.mem);
        end
    end
endmodule