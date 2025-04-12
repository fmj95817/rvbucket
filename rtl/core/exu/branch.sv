`include "isa.svh"

module exu_branch_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32i_inst_t inst,
    ex_rsp_if_t.mst    ex_rsp_mst,
    exu_gpr_r_if_t.mst gpr_r1_mst,
    exu_gpr_r_if_t.mst gpr_r2_mst,
    exu_gpr_w_if_t.mst gpr_w_mst,
    output logic       done
);
endmodule
