`include "isa.svh"

module exu_sys_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32i_inst_t inst,
    exu_gpr_r_if_t.mst gpr_r1_mst,
    exu_gpr_r_if_t.mst gpr_r2_mst,
    exu_gpr_w_if_t.mst gpr_w_mst
);
    assign gpr_r1_mst.vld = 1'b0;
    assign gpr_r1_mst.addr = {`RV_GPR_AW{1'b0}};

    assign gpr_r2_mst.vld = 1'b0;
    assign gpr_r2_mst.addr = {`RV_GPR_AW{1'b0}};

    assign gpr_w_mst.wen = 1'b0;
    assign gpr_w_mst.addr = {`RV_GPR_AW{1'b0}};
    assign gpr_w_mst.data = {`RV_XLEN{1'b0}};
endmodule