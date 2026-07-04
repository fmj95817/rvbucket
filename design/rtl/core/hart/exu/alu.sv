`include "spec/core/isa.svh"

module exu_alu_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32g_inst_t inst,
    exu_gpr_if_t.mst   gpr_mst
);
    localparam ALU_OP_SIZE = 5;
    localparam ALU_OP_ADD = 5'd0;
    localparam ALU_OP_SUB = 5'd1;
    localparam ALU_OP_LESS_S = 5'd2;
    localparam ALU_OP_LESS_U = 5'd3;
    localparam ALU_OP_XOR = 5'd4;
    localparam ALU_OP_OR = 5'd5;
    localparam ALU_OP_AND = 5'd6;
    localparam ALU_OP_SL = 5'd7;
    localparam ALU_OP_SRL = 5'd8;
    localparam ALU_OP_SRA = 5'd9;
    localparam ALU_OP_MUL = 5'd10;
    localparam ALU_OP_MULH = 5'd11;
    localparam ALU_OP_MULHSU = 5'd12;
    localparam ALU_OP_MULHU = 5'd13;
    localparam ALU_OP_DIV = 5'd14;
    localparam ALU_OP_DIVU = 5'd15;
    localparam ALU_OP_REM = 5'd16;
    localparam ALU_OP_REMU = 5'd17;

    logic [`RV_XLEN-1:0] alu_src1;
    logic [`RV_XLEN-1:0] alu_src2;
    logic [`RV_XLEN-1:0] alu_dst;
    logic [ALU_OP_SIZE-1:0] alu_op;

    tri signed [`RV_XLEN-1:0] alu_src1_s = alu_src1;
    tri signed [`RV_XLEN-1:0] alu_src2_s = alu_src2;
    tri [`RV_XLEN-1:0] alu_src1_u = alu_src1;
    tri [`RV_XLEN-1:0] alu_src2_u = alu_src2;
    tri [4:0] shift_bits = alu_src2[4:0];
    logic signed [63:0] mul_ss;
    logic signed [63:0] mul_su;
    logic [63:0] mul_uu;

    always_comb begin
        mul_ss = alu_src1_s * alu_src2_s;
        mul_su = alu_src1_s * $signed({1'b0, alu_src2_u});
        mul_uu = alu_src1_u * alu_src2_u;
    end

    always_comb begin
        case (alu_op)
            ALU_OP_ADD: alu_dst = alu_src1_u + alu_src2_u;
            ALU_OP_SUB: alu_dst = alu_src1_u - alu_src2_u;
            ALU_OP_LESS_S: alu_dst = alu_src1_s < alu_src2_s ?
                `RV_XLEN'd1 : `RV_XLEN'd0;
            ALU_OP_LESS_U: alu_dst = alu_src1_u < alu_src2_u ?
                `RV_XLEN'd1 : `RV_XLEN'd0;
            ALU_OP_XOR: alu_dst = alu_src1_u ^ alu_src2_u;
            ALU_OP_OR: alu_dst = alu_src1_u | alu_src2_u;
            ALU_OP_AND: alu_dst = alu_src1_u & alu_src2_u;
            ALU_OP_SL: alu_dst = alu_src1_u << shift_bits;
            ALU_OP_SRL: alu_dst = alu_src1_u >> shift_bits;
            ALU_OP_SRA: alu_dst = alu_src1_s >>> shift_bits;
            ALU_OP_MUL: alu_dst = mul_uu[31:0];
            ALU_OP_MULH: alu_dst = mul_ss[63:32];
            ALU_OP_MULHSU: alu_dst = mul_su[63:32];
            ALU_OP_MULHU: alu_dst = mul_uu[63:32];
            ALU_OP_DIV: begin
                if (alu_src2_u == 0)
                    alu_dst = 32'hffffffff;
                else if (alu_src1_u == 32'h80000000 && alu_src2_u == 32'hffffffff)
                    alu_dst = 32'h80000000;
                else
                    alu_dst = alu_src1_s / alu_src2_s;
            end
            ALU_OP_DIVU: alu_dst = alu_src2_u == 0 ? 32'hffffffff : alu_src1_u / alu_src2_u;
            ALU_OP_REM: begin
                if (alu_src2_u == 0)
                    alu_dst = alu_src1_u;
                else if (alu_src1_u == 32'h80000000 && alu_src2_u == 32'hffffffff)
                    alu_dst = 0;
                else
                    alu_dst = alu_src1_s % alu_src2_s;
            end
            ALU_OP_REMU: alu_dst = alu_src2_u == 0 ? alu_src1_u : alu_src1_u % alu_src2_u;
            default: alu_dst = {`RV_XLEN{1'bx}};
        endcase
    end

    logic [`RV_XLEN-1:0] i_imm;
    i_imm_decode u_i_imm_decode(
        .inst  (inst),
        .i_imm (i_imm)
    );

    always_comb begin
        gpr_mst.ra1 = {`RV_GPR_AW{1'bx}};
        gpr_mst.ra2 = {`RV_GPR_AW{1'bx}};
        gpr_mst.wen = 1'b0;
        gpr_mst.wa = {`RV_GPR_AW{1'bx}};
        gpr_mst.wd = {`RV_XLEN{1'bx}};
        alu_src1 = {`RV_XLEN{1'bx}};
        alu_src2 = {`RV_XLEN{1'bx}};
        alu_op = {ALU_OP_SIZE{1'bx}};

        if (sel && (inst.base.opcode == OPCODE_ALUI)) begin
            gpr_mst.ra1 = inst.i.rs1;
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = inst.i.rd;
            gpr_mst.wd = alu_dst;
            alu_src1 = gpr_mst.rd1;
            alu_src2 = i_imm;
            case (inst.i.funct3)
                ALUI_FUNCT3_ADDI: alu_op = ALU_OP_ADD;
                ALUI_FUNCT3_SLTI: alu_op = ALU_OP_LESS_S;
                ALUI_FUNCT3_SLTIU: alu_op = ALU_OP_LESS_U;
                ALUI_FUNCT3_XORI: alu_op = ALU_OP_XOR;
                ALUI_FUNCT3_ORI: alu_op = ALU_OP_OR;
                ALUI_FUNCT3_ANDI: alu_op = ALU_OP_AND;
                ALUI_FUNCT3_SLI: alu_op = ALU_OP_SL;
                ALUI_FUNCT3_SRI: alu_op = inst.i.imm_11_0[10] ?
                    ALU_OP_SRA : ALU_OP_SRL;
                default: alu_op = {ALU_OP_SIZE{1'bx}};
            endcase
        end else if (sel && (inst.base.opcode == OPCODE_ALU)) begin
            gpr_mst.ra1 = inst.r.rs1;
            gpr_mst.ra2 = inst.r.rs2;
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = inst.r.rd;
            gpr_mst.wd = alu_dst;
            alu_src1 = gpr_mst.rd1;
            alu_src2 = gpr_mst.rd2;
            if (inst.r.funct7 == ALU_FUNCT7_M) begin
                case (inst.r.funct3)
                    3'b000: alu_op = ALU_OP_MUL;
                    3'b001: alu_op = ALU_OP_MULH;
                    3'b010: alu_op = ALU_OP_MULHSU;
                    3'b011: alu_op = ALU_OP_MULHU;
                    3'b100: alu_op = ALU_OP_DIV;
                    3'b101: alu_op = ALU_OP_DIVU;
                    3'b110: alu_op = ALU_OP_REM;
                    3'b111: alu_op = ALU_OP_REMU;
                endcase
            end else case (inst.r.funct3)
                ALU_FUNCT3_ADD_SUB: alu_op = inst.r.funct7[5] ?
                    ALU_OP_SUB : ALU_OP_ADD;
                ALU_FUNCT3_SL: alu_op = ALU_OP_SL;
                ALU_FUNCT3_SLT: alu_op = ALU_OP_LESS_S;
                ALU_FUNCT3_SLTU: alu_op = ALU_OP_LESS_U;
                ALU_FUNCT3_XOR: alu_op = ALU_OP_XOR;
                ALU_FUNCT3_SR: alu_op = inst.r.funct7[5] ?
                    ALU_OP_SRA : ALU_OP_SRL;
                ALU_FUNCT3_OR: alu_op = ALU_OP_OR;
                ALU_FUNCT3_AND: alu_op = ALU_OP_AND;
                default: alu_op = {ALU_OP_SIZE{1'bx}};
            endcase
        end
    end
endmodule
