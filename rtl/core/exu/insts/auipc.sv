`include "isa.svh"

module auipc_handler(
    input [`RV_PC_SIZE-1:0] pc,
    rv32i_u_dec_if.slave    u_dec,
    exu_dp_if.master        dp_ctrl
);
    assign dp_ctrl.gpr_waddr = u_dec.rd;
    assign dp_ctrl.gpr_wdata = dp_ctrl.alu_dst;
    assign dp_ctrl.gpr_wen = 1'b1;

    assign dp_ctrl.alu_opcode = ALU_OPCODE_ADD;
    assign dp_ctrl.alu_src1 = pc;
    assign dp_ctrl.alu_src2 = u_dec.imm;
endmodule


