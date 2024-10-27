`include "isa.svh"
`include "exu/dp.svh"

interface exu_dp_if;
    logic [`RV_GPR_AW-1:0]    gpr_raddr1;
    logic [`RV_XLEN-1:0]      gpr_rdata1;
    logic [`RV_GPR_AW-1:0]    gpr_raddr2;
    logic [`RV_XLEN-1:0]      gpr_rdata2;
    logic [`RV_GPR_AW-1:0]    gpr_waddr;
    logic [`RV_XLEN-1:0]      gpr_wdata;
    logic                     gpr_wen;

    logic [`ALU_OPC_SIZE-1:0] alu_opcode;
    logic [`RV_XLEN-1:0]      alu_src1;
    logic [`RV_XLEN-1:0]      alu_src2;
    logic [`RV_XLEN-1:0]      alu_dst;

    modport master (
        output gpr_raddr1, gpr_raddr2, gpr_waddr, gpr_wdata, gpr_wen,
        input gpr_rdata1, gpr_rdata2,
        output alu_opcode, alu_src1, alu_src2, input alu_dst
    );

    modport slave (
        input gpr_raddr1, gpr_raddr2, gpr_waddr, gpr_wdata, gpr_wen,
        output gpr_rdata1, gpr_rdata2,
        input alu_opcode, alu_src1, alu_src2, output alu_dst
    );
endinterface

module exu_dp(
    input           clk,
    input           rst_n,
    exu_dp_if.slave dp_ctrl
);
    /* GPR */
    logic [`RV_XLEN-1:0] gprs[0:2**`RV_GPR_AW-1];
    assign gprs[0] = {`RV_XLEN{1'b0}};

    assign dp_ctrl.gpr_rdata1 = gprs[dp_ctrl.gpr_raddr1];
    assign dp_ctrl.gpr_rdata2 = gprs[dp_ctrl.gpr_raddr2];

    for (genvar i = 1; i < 2**`RV_GPR_AW; i++) begin : GEN_GPR
        always_ff @(posedge clk) begin
            if ((i == dp_ctrl.gpr_waddr) & dp_ctrl.gpr_wen)
                gprs[i] <= #1 dp_ctrl.gpr_wdata;
        end
    end

    /* ALU */
    tri signed [`RV_XLEN-1:0] alu_src1_s = dp_ctrl.alu_src1;
    tri signed [`RV_XLEN-1:0] alu_src2_s = dp_ctrl.alu_src2;
    tri  [`RV_XLEN-1:0] alu_src1_u = dp_ctrl.alu_src1;
    tri  [`RV_XLEN-1:0] alu_src2_u = dp_ctrl.alu_src2;
    tri [4:0] shift_bits = dp_ctrl.alu_src2[4:0];

    always_comb begin
        case (dp_ctrl.alu_opcode)
            ALU_OPCODE_ADD:
                dp_ctrl.alu_dst = alu_src1_u + alu_src2_u;
            ALU_OPCODE_SUB:
                dp_ctrl.alu_dst = alu_src1_u - alu_src2_u;
            ALU_OPCODE_LESS_S:
                dp_ctrl.alu_dst = alu_src1_s < alu_src2_s ?
                    `RV_XLEN'd1 : `RV_XLEN'd0;
            ALU_OPCODE_LESS_U:
                dp_ctrl.alu_dst = alu_src1_u < alu_src2_u ?
                    `RV_XLEN'd1 : `RV_XLEN'd0;
            ALU_OPCODE_XOR:
                dp_ctrl.alu_dst = alu_src1_u ^ alu_src2_u;
            ALU_OPCODE_OR:
                dp_ctrl.alu_dst = alu_src1_u | alu_src2_u;
            ALU_OPCODE_AND:
                dp_ctrl.alu_dst = alu_src1_u & alu_src2_u;
            ALU_OPCODE_SL:
                dp_ctrl.alu_dst = alu_src1_u << shift_bits;
            ALU_OPCODE_SRL:
                dp_ctrl.alu_dst = alu_src1_u >> shift_bits;
            ALU_OPCODE_SRA:
                dp_ctrl.alu_dst = alu_src1_s >> shift_bits;
        endcase
    end
endmodule

