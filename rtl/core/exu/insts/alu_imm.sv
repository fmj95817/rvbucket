`include "isa.svh"
`include "exu/dp.svh"

module alu_imm_handler(
    rv32i_i_dec_if.slave i_dec,
    exu_dp_if.master     dp_ctrl
);
    logic [`ALU_OPC_SIZE] alu_opcode;
    always_comb begin
        case (i_dec.funct3)
            ALU_IMM_FUNCT3_ADDI:
                alu_opcode = ALU_OPCODE_ADD;
            ALU_IMM_FUNCT3_SLTI:
                alu_opcode = ALU_OPCODE_LESS_S;
            ALU_IMM_FUNCT3_SLTIU:
                alu_opcode = ALU_OPCODE_LESS_U;
            ALU_IMM_FUNCT3_XORI:
                alu_opcode = ALU_OPCODE_XOR;
            ALU_IMM_FUNCT3_ORI:
                alu_opcode = ALU_OPCODE_OR;
            ALU_IMM_FUNCT3_ANDI:
                alu_opcode = ALU_OPCODE_AND;
            ALU_IMM_FUNCT3_SLI:
                alu_opcode = ALU_OPCODE_SL;
            ALU_IMM_FUNCT3_SRI:
                alu_opcode = i_dec.imm_11_0[10] ?
                    ALU_OPCODE_SRA : ALU_OPCODE_SRL;
            default:
                alu_opcode = {`ALU_OPC_SIZE{1'bx}};
        endcase
    end

    assign dp_ctrl.gpr_raddr1 = i_dec.rs1;
    assign dp_ctrl.gpr_waddr = i_dec.rd;
    assign dp_ctrl.gpr_wdata = dp_ctrl.alu_dst;
    assign dp_ctrl.gpr_wen = 1'b1;

    assign dp_ctrl.alu_opcode = alu_opcode;
    assign dp_ctrl.alu_src1 = dp_ctrl.gpr_rdata1;
    assign dp_ctrl.alu_src2 = i_dec.imm;
endmodule


