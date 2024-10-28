`include "isa.svh"
`include "boot.svh"

module sim_top(
    input clk,
    input rst_n
);
    localparam ROM_AW = 15;

    tri [`BOOT_ROM_AW-1:0] rom_addr;
    tri [31:0]             rom_data;

    bus_trans_if #(`RV_AW, `RV_XLEN) bti();

    rv32i u_rv32i(.*);
    bti_rom #(ROM_AW, `RV_XLEN) u_bti_rom(.*);

    initial begin
        $dumpfile("sim.vcd");
        $dumpvars(0, sim_top);
    end

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_bti_rom.u_rom.data);
        end
    end
endmodule