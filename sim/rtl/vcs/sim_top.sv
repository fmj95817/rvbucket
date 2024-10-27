`include "isa.svh"
`include "boot.svh"

module sim_top;
    localparam ROM_AW = 15;

    tri                    clk;
    tri                    rst_n;
    tri [`BOOT_ROM_AW-1:0] rom_addr;
    tri [31:0]             rom_data;

    bus_trans_if #(`RV_AW, `RV_XLEN) bti();

    clk_rst u_clk_rst(.*);
    rv32i u_rv32i(.*);
    bti_rom #(ROM_AW, `RV_XLEN) u_bti_rom(.*);

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_bti_rom.u_rom.data);
        end
    end
endmodule