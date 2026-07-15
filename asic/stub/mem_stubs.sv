`include "spec/core/isa.svh"
`include "boot.svh"

module boot_rom(
    input logic                         clk,
    input logic                         cs,
    input logic [`BOOT_ROM_WORD_AW-1:0] addr,
    output logic [`RV_XLEN-1:0]         data
) /* synthesis syn_black_box */;
endmodule

module rom #(
    parameter AW = 15,
    parameter DW = 32
)(
    input logic          clk,
    input logic          cs,
    input logic [AW-1:0] addr,
    output logic [DW-1:0] data
) /* synthesis syn_black_box */;
endmodule

module sram #(
    parameter AW = 15,
    parameter DW = 32
)(
    input logic      clk,
    sram_rw_if_t.slv sram_rw_slv
) /* synthesis syn_black_box */;
endmodule

module dp_sram #(
    parameter AW = 15,
    parameter DW = 32
)(
    input logic      clk,
    sram_rw_if_t.slv sram_r_slv,
    sram_rw_if_t.slv sram_w_slv
) /* synthesis syn_black_box */;
endmodule

module axi_ddr #(
    parameter DDR_AW = 28
)(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  axi4_aw_slv,
    axi4_w_if_t.slv   axi4_w_slv,
    axi4_b_if_t.mst   axi4_b_mst,
    axi4_ar_if_t.slv  axi4_ar_slv,
    axi4_r_if_t.mst   axi4_r_mst
) /* synthesis syn_black_box */;
endmodule
