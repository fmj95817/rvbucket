`include "isa.svh"

module exu_branch_handler(
    input logic                   clk,
    input logic                   rst_n,
    input logic                   sel,
    input rv32i_inst_t            inst,
    input logic [`RV_PC_SIZE-1:0] pc,
    input logic                   pred_taken,
    input logic [`RV_PC_SIZE-1:0] pred_pc,
    ex_rsp_if_t.mst               ex_rsp_mst,
    exu_gpr_r_if_t.mst            gpr_r1_mst,
    exu_gpr_r_if_t.mst            gpr_r2_mst,
    exu_gpr_w_if_t.mst            gpr_w_mst,
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

    tri [`RV_PC_SIZE-1:0] comp_src1 = gpr_r1_mst.data;
    tri [`RV_PC_SIZE-1:0]  comp_src2 = gpr_r2_mst.data;
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
        gpr_r1_mst.vld = 1'b0;
        gpr_r1_mst.addr = {`RV_GPR_AW{1'bx}};

        gpr_r2_mst.vld = 1'b0;
        gpr_r2_mst.addr = {`RV_GPR_AW{1'bx}};

        gpr_w_mst.wen = 1'b0;
        gpr_w_mst.addr = {`RV_GPR_AW{1'bx}};
        gpr_w_mst.data = {`RV_XLEN{1'bx}};

        ex_rsp_mst.pkt.taken = 1'b0;
        ex_rsp_mst.pkt.target_pc = {`RV_PC_SIZE{1'bx}};

        if (sel && (inst.base.opcode == OPCODE_JAL)) begin
            gpr_w_mst.wen = 1'b1;
            gpr_w_mst.addr = inst.j.rd;
            gpr_w_mst.data = link_pc;

            ex_rsp_mst.pkt.taken = 1'b1;
            ex_rsp_mst.pkt.target_pc = pc + j_imm;
        end else if (sel && (inst.base.opcode == OPCODE_JALR)) begin
            gpr_r1_mst.vld = 1'b1;
            gpr_r1_mst.addr = inst.i.rs1;

            gpr_w_mst.wen = 1'b1;
            gpr_w_mst.addr = inst.i.rd;
            gpr_w_mst.data = link_pc;

            ex_rsp_mst.pkt.taken = 1'b1;
            ex_rsp_mst.pkt.target_pc = gpr_r1_mst.data + i_imm;
        end else if (sel && (inst.base.opcode == OPCODE_BRANCH)) begin
            gpr_r1_mst.vld = 1'b1;
            gpr_r1_mst.addr = inst.b.rs1;

            gpr_r2_mst.vld = 1'b1;
            gpr_r2_mst.addr = inst.b.rs2;

            ex_rsp_mst.pkt.taken = comp_true;
            ex_rsp_mst.pkt.target_pc = pc + (comp_true ? b_imm : `RV_PC_SIZE'd4);
        end
    end

    assign ex_rsp_mst.pkt.pc = pc;
    always_comb begin
        if (ex_rsp_mst.pkt.taken)
            ex_rsp_mst.pkt.pred_true = pred_taken &&
                (ex_rsp_mst.pkt.target_pc == pred_pc);
        else
            ex_rsp_mst.pkt.pred_true = !pred_taken;
    end

    assign ex_rsp_mst.vld = sel;
    assign done = ex_rsp_mst.vld & ex_rsp_mst.rdy;

endmodule
