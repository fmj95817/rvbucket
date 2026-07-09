`include "spec/core/isa.svh"

module exu_branch_handler(
    input logic                   clk,
    input logic                   rst_n,
    input logic                   sel,
    input rv32g_inst_t            inst,
    input logic [`RV_PC_SIZE-1:0] pc,
    input logic                   pred_taken,
    input logic [`RV_PC_SIZE-1:0] pred_pc,
    ex_rsp_if_t.mst               ex_rsp_mst,
    exu_gpr_if_t.mst              gpr_mst,
    output logic                  done
);
    typedef enum logic [1:0] {IDLE, CALC, RESP} state_t;

    state_t state;
    rv32g_inst_t req_inst;
    logic [`RV_PC_SIZE-1:0] req_pc;
    logic req_pred_taken;
    logic [`RV_PC_SIZE-1:0] req_pred_pc;
    logic [`RV_XLEN-1:0] req_rd1;
    logic [`RV_XLEN-1:0] req_rd2;
    logic rsp_taken;
    logic [`RV_PC_SIZE-1:0] rsp_target_pc;
    logic [`RV_PC_SIZE-1:0] rsp_pc;
    logic rsp_pred_true;
    logic rsp_gpr_wen;
    logic [`RV_GPR_AW-1:0] rsp_gpr_wa;
    logic [`RV_XLEN-1:0] rsp_gpr_wd;

    logic [`RV_XLEN-1:0] req_i_imm;
    logic [`RV_XLEN-1:0] req_j_imm;
    logic [`RV_XLEN-1:0] req_b_imm;

    i_imm_decode u_i_imm_decode(
        .inst  (req_inst),
        .i_imm (req_i_imm)
    );

    j_imm_decode u_j_imm_decode(
        .inst  (req_inst),
        .j_imm (req_j_imm)
    );

    b_imm_decode u_b_imm_decode(
        .inst  (req_inst),
        .b_imm (req_b_imm)
    );

    wire signed [`RV_PC_SIZE-1:0] comp_src1_s = req_rd1;
    wire signed [`RV_PC_SIZE-1:0] comp_src2_s = req_rd2;
    logic comp_true;

    always_comb begin
        case (req_inst.b.funct3)
            BRANCH_FUNCT3_BEQ: comp_true = (req_rd1 == req_rd2);
            BRANCH_FUNCT3_BNE: comp_true = (req_rd1 != req_rd2);
            BRANCH_FUNCT3_BLT: comp_true = (comp_src1_s < comp_src2_s);
            BRANCH_FUNCT3_BGE: comp_true = (comp_src1_s >= comp_src2_s);
            BRANCH_FUNCT3_BLTU: comp_true = (req_rd1 < req_rd2);
            BRANCH_FUNCT3_BGEU: comp_true = (req_rd1 >= req_rd2);
            default: comp_true = 1'b0;
        endcase
    end

    wire [`RV_PC_SIZE-1:0] req_link_pc = req_pc + `RV_PC_SIZE'd4;
    wire rsp_hsk = ex_rsp_mst.vld && ex_rsp_mst.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            req_inst <= '0;
            req_pc <= {`RV_PC_SIZE{1'b0}};
            req_pred_taken <= 1'b0;
            req_pred_pc <= {`RV_PC_SIZE{1'b0}};
            req_rd1 <= {`RV_XLEN{1'b0}};
            req_rd2 <= {`RV_XLEN{1'b0}};
            rsp_taken <= 1'b0;
            rsp_target_pc <= {`RV_PC_SIZE{1'b0}};
            rsp_pc <= {`RV_PC_SIZE{1'b0}};
            rsp_pred_true <= 1'b0;
            rsp_gpr_wen <= 1'b0;
            rsp_gpr_wa <= {`RV_GPR_AW{1'b0}};
            rsp_gpr_wd <= {`RV_XLEN{1'b0}};
        end else begin
            case (state)
                IDLE: begin
                    if (sel) begin
                        req_inst <= inst;
                        req_pc <= pc;
                        req_pred_taken <= pred_taken;
                        req_pred_pc <= pred_pc;
                        req_rd1 <= gpr_mst.rd1;
                        req_rd2 <= gpr_mst.rd2;
                        state <= CALC;
                    end
                end
                CALC: begin
                    rsp_pc <= req_pc;
                    rsp_gpr_wen <= 1'b0;
                    rsp_gpr_wa <= {`RV_GPR_AW{1'b0}};
                    rsp_gpr_wd <= {`RV_XLEN{1'b0}};

                    case (req_inst.base.opcode)
                        OPCODE_JAL: begin
                            rsp_taken <= 1'b1;
                            rsp_target_pc <= req_pc + req_j_imm;
                            rsp_pred_true <= req_pred_taken &&
                                ((req_pc + req_j_imm) == req_pred_pc);
                            rsp_gpr_wen <= 1'b1;
                            rsp_gpr_wa <= req_inst.j.rd;
                            rsp_gpr_wd <= req_link_pc;
                        end
                        OPCODE_JALR: begin
                            rsp_taken <= 1'b1;
                            rsp_target_pc <= (req_rd1 + req_i_imm) &
                                32'hfffffffe;
                            rsp_pred_true <= req_pred_taken &&
                                (((req_rd1 + req_i_imm) &
                                32'hfffffffe) == req_pred_pc);
                            rsp_gpr_wen <= 1'b1;
                            rsp_gpr_wa <= req_inst.i.rd;
                            rsp_gpr_wd <= req_link_pc;
                        end
                        OPCODE_BRANCH: begin
                            rsp_taken <= comp_true;
                            rsp_target_pc <= req_pc +
                                (comp_true ? req_b_imm : `RV_PC_SIZE'd4);
                            if (comp_true) begin
                                rsp_pred_true <= req_pred_taken &&
                                    ((req_pc + req_b_imm) == req_pred_pc);
                            end else begin
                                rsp_pred_true <= !req_pred_taken;
                            end
                        end
                        default: begin
                            rsp_taken <= 1'b0;
                            rsp_target_pc <= {`RV_PC_SIZE{1'b0}};
                            rsp_pred_true <= 1'b0;
                        end
                    endcase
                    state <= RESP;
                end
                RESP: begin
                    if (rsp_hsk) begin
                        state <= IDLE;
                        rsp_gpr_wen <= 1'b0;
                        rsp_gpr_wa <= {`RV_GPR_AW{1'b0}};
                        rsp_gpr_wd <= {`RV_XLEN{1'b0}};
                    end
                end
                default: begin
                    state <= IDLE;
                end
            endcase
        end
    end

    always_comb begin
        gpr_mst.ra1 = {`RV_GPR_AW{1'bx}};
        gpr_mst.ra2 = {`RV_GPR_AW{1'bx}};
        gpr_mst.wen = state == RESP && rsp_gpr_wen && rsp_hsk;
        gpr_mst.wa = rsp_gpr_wa;
        gpr_mst.wd = rsp_gpr_wd;

        if (state == IDLE && sel && (inst.base.opcode == OPCODE_JALR)) begin
            gpr_mst.ra1 = inst.i.rs1;
        end else if (state == IDLE && sel &&
            (inst.base.opcode == OPCODE_BRANCH)) begin
            gpr_mst.ra1 = inst.b.rs1;
            gpr_mst.ra2 = inst.b.rs2;
        end
    end

    assign ex_rsp_mst.vld = state == RESP;
    assign ex_rsp_mst.pkt.taken = rsp_taken;
    assign ex_rsp_mst.pkt.target_pc = rsp_target_pc;
    assign ex_rsp_mst.pkt.pc = rsp_pc;
    assign ex_rsp_mst.pkt.pred_true = rsp_pred_true;
    assign done = rsp_hsk;

endmodule
