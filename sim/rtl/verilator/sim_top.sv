`include "isa.svh"

module sim_top(
    input logic clk,
    input logic rst_n
);
    soc u_soc(
        .clk   (clk),
        .rst_n (rst_n)
    );

    initial begin
        $dumpfile("sim.vcd");
        $dumpvars(0, sim_top);
    end

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_soc.u_flash.u_rom.mem);
        end
    end
endmodule