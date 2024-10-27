`include "isa.svh"
`include "exu/dp.svh"

module exu_dp_mux(
    input [`RV_OPC_SIZE-1:0] opcode,
    input                    iexec_req_hsk,
    exu_dp_if.slave          lui_dp_op,
    exu_dp_if.slave          alu_imm_dp_op,
    exu_dp_if.master         dp_op
);
    logic gpr_wen;
    assign dp_op.gpr_wen = gpr_wen & iexec_req_hsk;

    always_comb begin
        dp_op.alu_opcode = {`ALU_OPC_SIZE{1'bx}};
        dp_op.alu_src1 = {`RV_XLEN{1'bx}};
        dp_op.alu_src2 = {`RV_XLEN{1'bx}};

        dp_op.gpr_raddr1 = {`RV_GPR_AW{1'bx}};
        dp_op.gpr_raddr2 = {`RV_GPR_AW{1'bx}};
        dp_op.gpr_waddr = {`RV_GPR_AW{1'bx}};
        dp_op.gpr_wdata = {`RV_XLEN{1'bx}};
        gpr_wen = 1'b0;

        case (opcode)
            OPCODE_LUI: begin
                dp_op.gpr_waddr = lui_dp_op.gpr_waddr;
                dp_op.gpr_wdata = lui_dp_op.gpr_wdata;
                gpr_wen = lui_dp_op.gpr_wen;
            end
            OPCODE_ALU_IMM: begin
                dp_op.gpr_raddr1 = alu_imm_dp_op.gpr_raddr1;
                dp_op.gpr_waddr = alu_imm_dp_op.gpr_waddr;
                dp_op.gpr_wdata = alu_imm_dp_op.gpr_wdata;
                gpr_wen = alu_imm_dp_op.gpr_wen;

                dp_op.alu_opcode = alu_imm_dp_op.alu_opcode;
                dp_op.alu_src1 = alu_imm_dp_op.alu_src1;
                dp_op.alu_src2 = alu_imm_dp_op.alu_src2;
            end
        endcase
    end

    /* OPCODE_ALU_IMM */
    assign alu_imm_dp_op.gpr_rdata1 = dp_op.gpr_rdata1;
    assign alu_imm_dp_op.alu_dst = dp_op.alu_dst;
endmodule