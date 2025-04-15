`include "isa.svh"

module exu_misc_handler(
    input logic                   clk,
    input logic                   rst_n,
    input logic                   sel,
    input rv32i_inst_t            inst,
    input logic [`RV_PC_SIZE-1:0] pc,
    exu_gpr_r_if_t.mst            gpr_r1_mst,
    exu_gpr_r_if_t.mst            gpr_r2_mst,
    exu_gpr_w_if_t.mst            gpr_w_mst
);
    logic [`RV_XLEN-1:0] u_imm;
    u_imm_decode u_u_imm_decode(
        .inst  (inst),
        .u_imm (u_imm)
    );

    assign gpr_r1_mst.vld = 1'b0;
    assign gpr_r1_mst.addr = {`RV_GPR_AW{1'b0}};

    assign gpr_r2_mst.vld = 1'b0;
    assign gpr_r2_mst.addr = {`RV_GPR_AW{1'b0}};

    assign gpr_w_mst.wen = sel;
    assign gpr_w_mst.addr = inst.u.rd;
    always_comb begin
        if (inst.base.opcode == OPCODE_LUI)
            gpr_w_mst.data = u_imm;
        else if (inst.base.opcode == OPCODE_AUIPC)
            gpr_w_mst.data = pc + u_imm;
        else
            gpr_w_mst.data = {`RV_XLEN{1'bx}};
    end

endmodule