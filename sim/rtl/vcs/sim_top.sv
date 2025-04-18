module sim_top;
    localparam ITCM_AW = 15;
    localparam DTCM_AW = 15;

    tri clk;
    tri rst_n;

    clk_rst u_clk_rst(
        .clk   (clk),
        .rst_n (rst_n)
    );

    soc u_soc(
        .clk   (clk),
        .rst_n (rst_n)
    );

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_soc.u_itcm.u_rom.mem);
        end

        $fsdbDumpfile("sim_top.fsdb");
        $fsdbDumpvars(0, sim_top, "+all");
    end
endmodule