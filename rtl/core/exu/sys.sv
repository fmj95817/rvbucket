`include "isa.svh"

module exu_sys_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32i_inst_t inst,
    exu_gpr_if_t.mst   gpr_mst
);
    assign gpr_mst.ra1 = {`RV_GPR_AW{1'b0}};
    assign gpr_mst.ra2 = {`RV_GPR_AW{1'b0}};
    assign gpr_mst.wen = 1'b0;
    assign gpr_mst.wa = {`RV_GPR_AW{1'b0}};
    assign gpr_mst.wd = {`RV_XLEN{1'b0}};
endmodule