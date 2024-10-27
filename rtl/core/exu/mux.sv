`include "isa.svh"
`include "exu/dp.svh"

module exu_dp_mux(
    input                    iexec_req_hsk,
    rv32i_opc_dec_if.slave   opc_dec,
    exu_dp_if.slave          lui_dp_ctrl,
    exu_dp_if.slave          auipc_dp_ctrl,
    exu_dp_if.slave          alu_imm_dp_ctrl,
    exu_dp_if.master         dp_ctrl
);
    assign dp_ctrl.gpr_raddr1 = (
        ({`RV_GPR_AW{opc_dec.alu_imm}} & alu_imm_dp_ctrl.gpr_raddr1)
    );

    assign dp_ctrl.gpr_waddr = (
        ({`RV_GPR_AW{opc_dec.lui}} & lui_dp_ctrl.gpr_waddr) |
        ({`RV_GPR_AW{opc_dec.auipc}} & auipc_dp_ctrl.gpr_waddr) |
        ({`RV_GPR_AW{opc_dec.alu_imm}} & alu_imm_dp_ctrl.gpr_waddr)
    );

    assign dp_ctrl.gpr_wdata = (
        ({`RV_XLEN{opc_dec.lui}} & lui_dp_ctrl.gpr_wdata) |
        ({`RV_XLEN{opc_dec.auipc}} & auipc_dp_ctrl.gpr_wdata) |
        ({`RV_XLEN{opc_dec.alu_imm}} & alu_imm_dp_ctrl.gpr_wdata)
    );

    assign dp_ctrl.gpr_wen = iexec_req_hsk & (
        (opc_dec.lui & lui_dp_ctrl.gpr_wen) |
        (opc_dec.auipc & auipc_dp_ctrl.gpr_wen) |
        (opc_dec.alu_imm & alu_imm_dp_ctrl.gpr_wen)
    );

    assign dp_ctrl.alu_opcode = (
        ({`ALU_OPC_SIZE{opc_dec.auipc}} & auipc_dp_ctrl.alu_opcode) |
        ({`ALU_OPC_SIZE{opc_dec.alu_imm}} & alu_imm_dp_ctrl.alu_opcode)
    );

    assign dp_ctrl.alu_src1 = (
        ({`RV_XLEN{opc_dec.auipc}} & auipc_dp_ctrl.alu_src1) |
        ({`RV_XLEN{opc_dec.alu_imm}} & alu_imm_dp_ctrl.alu_src1)
    );

    assign dp_ctrl.alu_src2 = (
        ({`RV_XLEN{opc_dec.auipc}} & auipc_dp_ctrl.alu_src2) |
        ({`RV_XLEN{opc_dec.alu_imm}} & alu_imm_dp_ctrl.alu_src2)
    );

    /* AUIPC */
    assign auipc_dp_ctrl.alu_dst = dp_ctrl.alu_dst;

    /* OPCODE_ALU_IMM */
    assign alu_imm_dp_ctrl.gpr_rdata1 = dp_ctrl.gpr_rdata1;
    assign alu_imm_dp_ctrl.alu_dst = dp_ctrl.alu_dst;
endmodule