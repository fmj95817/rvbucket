`include "spec/core/isa.svh"

module exu_alu_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32g_inst_t inst,
    exu_gpr_if_t.mst   gpr_mst,
    output logic       done
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

    typedef enum logic [1:0] {
        M_STATE_IDLE,
        M_STATE_MUL,
        M_STATE_DIV
    } m_state_t;

    logic [`RV_XLEN-1:0] alu_src1;
    logic [`RV_XLEN-1:0] alu_src2;
    logic [ALU_OP_SIZE-1:0] alu_op;

    m_state_t m_state;
    logic [ALU_OP_SIZE-1:0] m_op;
    logic [`RV_GPR_AW-1:0] m_wa;
    logic [`RV_XLEN-1:0] m_src1;
    logic [`RV_XLEN-1:0] m_src2;
    logic [`RV_XLEN-1:0] m_result;
    logic m_done;

    logic [5:0] m_cnt;
    logic [63:0] mul_acc;
    logic [63:0] mul_mcand;
    logic [31:0] mul_mult;
    logic mul_neg;

    logic [31:0] div_divisor;
    logic [31:0] div_dividend;
    logic [31:0] div_quotient;
    logic [31:0] div_remainder;
    logic div_neg_quotient;
    logic div_neg_remainder;

    logic [31:0] start_abs_src1;
    logic [31:0] start_abs_src2;
    logic start_src1_neg;
    logic start_src2_neg;
    logic [31:0] mul_start_src1;
    logic [31:0] mul_start_src2;
    logic mul_start_neg;
    logic [63:0] mul_acc_add;
    logic [63:0] mul_product;
    logic [32:0] div_remainder_shift;
    logic [31:0] div_remainder_next;
    logic [31:0] div_quotient_next;

    function automatic logic [`RV_XLEN-1:0] alu_calc(
        input logic [ALU_OP_SIZE-1:0] op,
        input logic [`RV_XLEN-1:0] src1,
        input logic [`RV_XLEN-1:0] src2,
        input logic [`RV_XLEN-1:0] m_res
    );
        logic signed [`RV_XLEN-1:0] src1_s;
        logic signed [`RV_XLEN-1:0] src2_s;
        begin
            src1_s = src1;
            src2_s = src2;
            case (op)
                ALU_OP_ADD: alu_calc = src1 + src2;
                ALU_OP_SUB: alu_calc = src1 - src2;
                ALU_OP_LESS_S: alu_calc = src1_s < src2_s ?
                    `RV_XLEN'd1 : `RV_XLEN'd0;
                ALU_OP_LESS_U: alu_calc = src1 < src2 ?
                    `RV_XLEN'd1 : `RV_XLEN'd0;
                ALU_OP_XOR: alu_calc = src1 ^ src2;
                ALU_OP_OR: alu_calc = src1 | src2;
                ALU_OP_AND: alu_calc = src1 & src2;
                ALU_OP_SL: alu_calc = src1 << src2[4:0];
                ALU_OP_SRL: alu_calc = src1 >> src2[4:0];
                ALU_OP_SRA: alu_calc = src1_s >>> src2[4:0];
                ALU_OP_MUL,
                ALU_OP_MULH,
                ALU_OP_MULHSU,
                ALU_OP_MULHU,
                ALU_OP_DIV,
                ALU_OP_DIVU,
                ALU_OP_REM,
                ALU_OP_REMU: alu_calc = m_res;
                default: alu_calc = {`RV_XLEN{1'b0}};
            endcase
        end
    endfunction

    wire inst_m = inst.base.opcode == OPCODE_ALU &&
        inst.r.funct7 == ALU_FUNCT7_M;
    wire simple_fire = sel && !inst_m && m_state == M_STATE_IDLE;
    wire m_start = sel && inst_m && m_state == M_STATE_IDLE && !m_done;
    wire m_mul_start = m_start && inst.r.funct3 <= 3'b011;
    wire m_div_start = m_start && inst.r.funct3 >= 3'b100;

    always_comb begin
        start_src1_neg = gpr_mst.rd1[31];
        start_src2_neg = gpr_mst.rd2[31];
        start_abs_src1 = start_src1_neg ? (~gpr_mst.rd1 + 32'd1) :
            gpr_mst.rd1;
        start_abs_src2 = start_src2_neg ? (~gpr_mst.rd2 + 32'd1) :
            gpr_mst.rd2;
        mul_start_src1 = gpr_mst.rd1;
        mul_start_src2 = gpr_mst.rd2;
        mul_start_neg = 1'b0;

        case (inst.r.funct3)
            3'b001: begin
                mul_start_src1 = start_abs_src1;
                mul_start_src2 = start_abs_src2;
                mul_start_neg = start_src1_neg ^ start_src2_neg;
            end
            3'b010: begin
                mul_start_src1 = start_abs_src1;
                mul_start_src2 = gpr_mst.rd2;
                mul_start_neg = start_src1_neg;
            end
            default: ;
        endcase

        mul_acc_add = mul_mult[0] ? (mul_acc + mul_mcand) : mul_acc;
        mul_product = mul_neg ? (~mul_acc_add + 64'd1) : mul_acc_add;

        div_remainder_shift = {1'b0, div_remainder[30:0], div_dividend[31]};
        if (div_remainder_shift >= {1'b0, div_divisor}) begin
            div_remainder_next = div_remainder_shift[31:0] - div_divisor;
            div_quotient_next = {div_quotient[30:0], 1'b1};
        end else begin
            div_remainder_next = div_remainder_shift[31:0];
            div_quotient_next = {div_quotient[30:0], 1'b0};
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            m_state <= M_STATE_IDLE;
            m_op <= {ALU_OP_SIZE{1'b0}};
            m_wa <= {`RV_GPR_AW{1'b0}};
            m_src1 <= {`RV_XLEN{1'b0}};
            m_src2 <= {`RV_XLEN{1'b0}};
            m_result <= {`RV_XLEN{1'b0}};
            m_done <= 1'b0;
            m_cnt <= 6'd0;
            mul_acc <= 64'd0;
            mul_mcand <= 64'd0;
            mul_mult <= 32'd0;
            mul_neg <= 1'b0;
            div_divisor <= 32'd0;
            div_dividend <= 32'd0;
            div_quotient <= 32'd0;
            div_remainder <= 32'd0;
            div_neg_quotient <= 1'b0;
            div_neg_remainder <= 1'b0;
        end else begin
            if (m_done && sel)
                m_done <= 1'b0;

            case (m_state)
                M_STATE_IDLE: begin
                    if (m_mul_start) begin
                        m_state <= M_STATE_MUL;
                        m_op <= alu_op;
                        m_wa <= inst.r.rd;
                        m_src1 <= gpr_mst.rd1;
                        m_src2 <= gpr_mst.rd2;
                        m_cnt <= 6'd0;
                        mul_acc <= 64'd0;
                        mul_mcand <= {32'd0, mul_start_src1};
                        mul_mult <= mul_start_src2;
                        mul_neg <= mul_start_neg;
                    end else if (m_div_start) begin
                        m_op <= alu_op;
                        m_wa <= inst.r.rd;
                        m_src1 <= gpr_mst.rd1;
                        m_src2 <= gpr_mst.rd2;
                        if (gpr_mst.rd2 == 32'd0) begin
                            m_result <= inst.r.funct3[1] ?
                                gpr_mst.rd1 : 32'hffffffff;
                            m_done <= 1'b1;
                        end else if (!inst.r.funct3[0] &&
                            gpr_mst.rd1 == 32'h80000000 &&
                            gpr_mst.rd2 == 32'hffffffff) begin
                            m_result <= inst.r.funct3[1] ?
                                32'd0 : 32'h80000000;
                            m_done <= 1'b1;
                        end else begin
                            m_state <= M_STATE_DIV;
                            m_cnt <= 6'd0;
                            div_divisor <= inst.r.funct3[0] ?
                                gpr_mst.rd2 : start_abs_src2;
                            div_dividend <= inst.r.funct3[0] ?
                                gpr_mst.rd1 : start_abs_src1;
                            div_quotient <= 32'd0;
                            div_remainder <= 32'd0;
                            div_neg_quotient <= !inst.r.funct3[0] &&
                                (start_src1_neg ^ start_src2_neg);
                            div_neg_remainder <= !inst.r.funct3[0] &&
                                start_src1_neg;
                        end
                    end
                end
                M_STATE_MUL: begin
                    mul_acc <= mul_acc_add;
                    mul_mcand <= mul_mcand << 1;
                    mul_mult <= mul_mult >> 1;
                    m_cnt <= m_cnt + 6'd1;
                    if (m_cnt == 6'd31) begin
                        m_state <= M_STATE_IDLE;
                        case (m_op)
                            ALU_OP_MUL: m_result <= mul_product[31:0];
                            ALU_OP_MULH,
                            ALU_OP_MULHSU,
                            ALU_OP_MULHU: m_result <= mul_product[63:32];
                            default: m_result <= {`RV_XLEN{1'b0}};
                        endcase
                        m_done <= 1'b1;
                    end
                end
                M_STATE_DIV: begin
                    div_remainder <= div_remainder_next;
                    div_quotient <= div_quotient_next;
                    div_dividend <= {div_dividend[30:0], 1'b0};
                    m_cnt <= m_cnt + 6'd1;
                    if (m_cnt == 6'd31) begin
                        m_state <= M_STATE_IDLE;
                        case (m_op)
                            ALU_OP_DIV: begin
                                m_result <= div_neg_quotient ?
                                    (~div_quotient_next + 32'd1) :
                                    div_quotient_next;
                            end
                            ALU_OP_DIVU: m_result <= div_quotient_next;
                            ALU_OP_REM: begin
                                m_result <= div_neg_remainder ?
                                    (~div_remainder_next + 32'd1) :
                                    div_remainder_next;
                            end
                            ALU_OP_REMU: m_result <= div_remainder_next;
                            default: m_result <= {`RV_XLEN{1'b0}};
                        endcase
                        m_done <= 1'b1;
                    end
                end
                default: m_state <= M_STATE_IDLE;
            endcase
        end
    end

    logic [`RV_XLEN-1:0] i_imm;
    i_imm_decode u_i_imm_decode(
        .inst  (inst),
        .i_imm (i_imm)
    );

    always_comb begin
        gpr_mst.ra1 = {`RV_GPR_AW{1'b0}};
        gpr_mst.ra2 = {`RV_GPR_AW{1'b0}};
        gpr_mst.wen = 1'b0;
        gpr_mst.wa = {`RV_GPR_AW{1'b0}};
        gpr_mst.wd = {`RV_XLEN{1'b0}};
        alu_src1 = {`RV_XLEN{1'b0}};
        alu_src2 = {`RV_XLEN{1'b0}};
        alu_op = ALU_OP_ADD;

        done = 1'b0;

        if (m_done) begin
            done = 1'b1;
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = m_wa;
            gpr_mst.wd = m_result;
        end else if (sel && (inst.base.opcode == OPCODE_ALUI)) begin
            gpr_mst.ra1 = inst.i.rs1;
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
                default: alu_op = ALU_OP_ADD;
            endcase
        end else if (sel && (inst.base.opcode == OPCODE_ALU)) begin
            gpr_mst.ra1 = inst.r.rs1;
            gpr_mst.ra2 = inst.r.rs2;
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
            end else begin
                case (inst.r.funct3)
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
                    default: alu_op = ALU_OP_ADD;
                endcase
            end
        end

        if (simple_fire) begin
            done = 1'b1;
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = (inst.base.opcode == OPCODE_ALUI) ?
                inst.i.rd : inst.r.rd;
            gpr_mst.wd = alu_calc(alu_op, alu_src1, alu_src2, m_result);
        end
    end
endmodule
