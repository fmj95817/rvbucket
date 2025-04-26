`include "core/isa.svh"

module exu_misc_handler(
    input logic                   clk,
    input logic                   rst_n,
    input logic                   sel,
    input rv32i_inst_t            inst,
    input logic [`RV_PC_SIZE-1:0] pc,
    exu_gpr_if_t.mst              gpr_mst
);
    logic [`RV_XLEN-1:0] u_imm;
    u_imm_decode u_u_imm_decode(
        .inst  (inst),
        .u_imm (u_imm)
    );

    assign gpr_mst.ra1 = {`RV_GPR_AW{1'b0}};
    assign gpr_mst.ra2 = {`RV_GPR_AW{1'b0}};
    assign gpr_mst.wen = sel;
    assign gpr_mst.wa = inst.u.rd;
    always_comb begin
        if (inst.base.opcode == OPCODE_LUI)
            gpr_mst.wd = u_imm;
        else if (inst.base.opcode == OPCODE_AUIPC)
            gpr_mst.wd = pc + u_imm;
        else
            gpr_mst.wd = {`RV_XLEN{1'bx}};
    end

endmodule