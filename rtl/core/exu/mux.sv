`include "isa.svh"
`include "exu/dp.svh"

module exu_dp_mux(
    rv32i_opc_dec_if.slave   opc_dec,
    exu_dp_if.slave          lui_dp_ctrl,
    exu_dp_if.slave          auipc_dp_ctrl,
    exu_dp_if.slave          load_dp_ctrl,
    exu_dp_if.slave          alui_dp_ctrl,
    exu_dp_if.master         dp_ctrl
);
    assign dp_ctrl.gpr_raddr1 = (
        ({`RV_GPR_AW{opc_dec.load}} & load_dp_ctrl.gpr_raddr1) |
        ({`RV_GPR_AW{opc_dec.alui}} & alui_dp_ctrl.gpr_raddr1)
    );

    assign dp_ctrl.gpr_waddr = (
        ({`RV_GPR_AW{opc_dec.lui}} & lui_dp_ctrl.gpr_waddr) |
        ({`RV_GPR_AW{opc_dec.auipc}} & auipc_dp_ctrl.gpr_waddr) |
        ({`RV_GPR_AW{opc_dec.load}} & load_dp_ctrl.gpr_waddr) |
        ({`RV_GPR_AW{opc_dec.alui}} & alui_dp_ctrl.gpr_waddr)
    );

    assign dp_ctrl.gpr_wdata = (
        ({`RV_XLEN{opc_dec.lui}} & lui_dp_ctrl.gpr_wdata) |
        ({`RV_XLEN{opc_dec.auipc}} & auipc_dp_ctrl.gpr_wdata) |
        ({`RV_XLEN{opc_dec.load}} & load_dp_ctrl.gpr_wdata) |
        ({`RV_XLEN{opc_dec.alui}} & alui_dp_ctrl.gpr_wdata)
    );

    assign dp_ctrl.gpr_wen = (
        (opc_dec.lui & lui_dp_ctrl.gpr_wen) |
        (opc_dec.auipc & auipc_dp_ctrl.gpr_wen) |
        (opc_dec.load & load_dp_ctrl.gpr_wen) |
        (opc_dec.alui & alui_dp_ctrl.gpr_wen)
    );

    assign dp_ctrl.alu_opcode = (
        ({`ALU_OPC_SIZE{opc_dec.auipc}} & auipc_dp_ctrl.alu_opcode) |
        ({`ALU_OPC_SIZE{opc_dec.load}} & load_dp_ctrl.alu_opcode) |
        ({`ALU_OPC_SIZE{opc_dec.alui}} & alui_dp_ctrl.alu_opcode)
    );

    assign dp_ctrl.alu_src1 = (
        ({`RV_XLEN{opc_dec.auipc}} & auipc_dp_ctrl.alu_src1) |
        ({`RV_XLEN{opc_dec.load}} & load_dp_ctrl.alu_src1) |
        ({`RV_XLEN{opc_dec.alui}} & alui_dp_ctrl.alu_src1)
    );

    assign dp_ctrl.alu_src2 = (
        ({`RV_XLEN{opc_dec.auipc}} & auipc_dp_ctrl.alu_src2) |
        ({`RV_XLEN{opc_dec.load}} & load_dp_ctrl.alu_src2) |
        ({`RV_XLEN{opc_dec.alui}} & alui_dp_ctrl.alu_src2)
    );

    /* AUIPC */
    assign auipc_dp_ctrl.alu_dst = dp_ctrl.alu_dst;

    /* LOAD */
    assign load_dp_ctrl.gpr_rdata1 = dp_ctrl.gpr_rdata1;
    assign load_dp_ctrl.alu_dst = dp_ctrl.alu_dst;

    /* ALUI */
    assign alui_dp_ctrl.gpr_rdata1 = dp_ctrl.gpr_rdata1;
    assign alui_dp_ctrl.alu_dst = dp_ctrl.alu_dst;
endmodule

module exu_ldst_mux(
    rv32i_opc_dec_if.slave opc_dec,
    ldst_if.slave          load_ldst,
    ldst_if.master         ldst_src
);
endmodule

module exu_iexec_mux(
    rv32i_opc_dec_if.slave opc_dec,
    input                  load_iexec_req_rdy,
    output                 iexec_req_rdy
);
    assign iexec_req_rdy = opc_dec.load ? load_iexec_req_rdy : 1'b1;
endmodule