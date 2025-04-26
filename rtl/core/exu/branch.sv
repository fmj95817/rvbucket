`include "core/isa.svh"

module exu_branch_handler(
    input logic                   clk,
    input logic                   rst_n,
    input logic                   sel,
    input rv32i_inst_t            inst,
    input logic [`RV_PC_SIZE-1:0] pc,
    input logic                   pred_taken,
    input logic [`RV_PC_SIZE-1:0] pred_pc,
    ex_rsp_if_t.mst               ex_rsp_mst,
    exu_gpr_if_t.mst              gpr_mst,
    output logic                  done
);
    logic [`RV_XLEN-1:0] i_imm;
    logic [`RV_XLEN-1:0] j_imm;
    logic [`RV_XLEN-1:0] b_imm;

    i_imm_decode u_i_imm_decode(
        .inst  (inst),
        .i_imm (i_imm)
    );

    j_imm_decode u_j_imm_decode(
        .inst  (inst),
        .j_imm (j_imm)
    );

    b_imm_decode u_b_imm_decode(
        .inst  (inst),
        .b_imm (b_imm)
    );

    tri [`RV_PC_SIZE-1:0] comp_src1 = gpr_mst.rd1;
    tri [`RV_PC_SIZE-1:0]  comp_src2 = gpr_mst.rd2;
    tri signed [`RV_PC_SIZE-1:0] comp_src1_s = comp_src1;
    tri signed [`RV_PC_SIZE-1:0] comp_src2_s = comp_src2;
    logic comp_true;

    always_comb begin
        case (inst.b.funct3)
            BRANCH_FUNCT3_BEQ: comp_true = (comp_src1 == comp_src2);
            BRANCH_FUNCT3_BNE: comp_true = (comp_src1 != comp_src2);
            BRANCH_FUNCT3_BLT: comp_true = (comp_src1_s < comp_src2_s);
            BRANCH_FUNCT3_BGE: comp_true = (comp_src1_s >= comp_src2_s);
            BRANCH_FUNCT3_BLTU: comp_true = (comp_src1 < comp_src2);
            BRANCH_FUNCT3_BGEU: comp_true = (comp_src1 >= comp_src2);
            default: comp_true = 1'bx;
        endcase
    end

    tri [`RV_PC_SIZE-1:0] link_pc = pc + `RV_PC_SIZE'd4;
    always_comb begin
        gpr_mst.ra1 = {`RV_GPR_AW{1'bx}};
        gpr_mst.ra2 = {`RV_GPR_AW{1'bx}};
        gpr_mst.wen = 1'b0;
        gpr_mst.wa = {`RV_GPR_AW{1'bx}};
        gpr_mst.wd = {`RV_XLEN{1'bx}};
        ex_rsp_mst.pkt.taken = 1'b0;
        ex_rsp_mst.pkt.target_pc = {`RV_PC_SIZE{1'bx}};
        ex_rsp_mst.pkt.pc = pc;
        ex_rsp_mst.pkt.pred_true = 1'b0;

        if (sel && (inst.base.opcode == OPCODE_JAL)) begin
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = inst.j.rd;
            gpr_mst.wd = link_pc;
            ex_rsp_mst.pkt.taken = 1'b1;
            ex_rsp_mst.pkt.target_pc = pc + j_imm;
            ex_rsp_mst.pkt.pred_true = pred_taken && (ex_rsp_mst.pkt.target_pc == pred_pc);
        end else if (sel && (inst.base.opcode == OPCODE_JALR)) begin
            gpr_mst.ra1 = inst.i.rs1;
            gpr_mst.wen = 1'b1;
            gpr_mst.wa = inst.i.rd;
            gpr_mst.wd = link_pc;
            ex_rsp_mst.pkt.taken = 1'b1;
            ex_rsp_mst.pkt.target_pc = gpr_mst.rd1 + i_imm;
            ex_rsp_mst.pkt.pred_true = pred_taken && (ex_rsp_mst.pkt.target_pc == pred_pc);
        end else if (sel && (inst.base.opcode == OPCODE_BRANCH)) begin
            gpr_mst.ra1 = inst.b.rs1;
            gpr_mst.ra2 = inst.b.rs2;
            ex_rsp_mst.pkt.taken = comp_true;
            ex_rsp_mst.pkt.target_pc = pc + (comp_true ? b_imm : `RV_PC_SIZE'd4);
            if (comp_true)
                ex_rsp_mst.pkt.pred_true = pred_taken && (ex_rsp_mst.pkt.target_pc == pred_pc);
            else
                ex_rsp_mst.pkt.pred_true = !pred_taken;
        end
    end

    assign ex_rsp_mst.vld = sel;
    assign done = ex_rsp_mst.vld & ex_rsp_mst.rdy;

endmodule
