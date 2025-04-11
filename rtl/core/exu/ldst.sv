`include "isa.svh"

module exu_ldst_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32i_inst_t inst,
    ldst_req_if_t.mst  ldst_req_mst,
    ldst_rsp_if_t.slv  ldst_rsp_slv,
    exu_gpr_r_if_t.mst gpr_r1_mst,
    exu_gpr_r_if_t.mst gpr_r2_mst,
    exu_gpr_w_if_t.mst gpr_w_mst
);
endmodule