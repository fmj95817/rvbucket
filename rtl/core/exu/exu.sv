`include "isa.svh"

module exu(
    input logic       clk,
    input logic       rst_n,
    ex_req_if_t.slv   ex_req_slv,
    ex_rsp_if_t.mst   ex_rsp_mst,
    ldst_req_if_t.mst ldst_req_mst,
    ldst_rsp_if_t.slv ldst_rsp_slv
);
    localparam INST_HANDLER_NUM = 4;
    localparam ALU_CHN_IDX = 0;
    localparam BRANCH_CHN_IDX = 1;
    localparam LDST_CHN_IDX = 2;
    localparam MISC_CHN_IDX = 3;

    tri [`RV_OPC_SIZE-1:0] opcode = ex_req_slv.pkt.ir.base.opcode;
    tri is_lui = ex_req_slv.vld & (opcode == OPCODE_LUI);
    tri is_auipc = ex_req_slv.vld & (opcode == OPCODE_AUIPC);
    tri is_jal = ex_req_slv.vld & (opcode == OPCODE_JAL);
    tri is_jalr = ex_req_slv.vld & (opcode == OPCODE_JALR);
    tri is_branch = ex_req_slv.vld & (opcode == OPCODE_BRANCH);
    tri is_load = ex_req_slv.vld & (opcode == OPCODE_LOAD);
    tri is_store = ex_req_slv.vld & (opcode == OPCODE_STORE);
    tri is_alui = ex_req_slv.vld & (opcode == OPCODE_ALUI);
    tri is_alu = ex_req_slv.vld & (opcode == OPCODE_ALU);
    tri is_mem = ex_req_slv.vld & (opcode == OPCODE_MEM);
    tri is_system = ex_req_slv.vld & (opcode == OPCODE_SYSTEM);

    tri alu_sel = is_alu | is_alui;
    tri branch_sel = is_jal | is_jalr | is_branch;
    tri ldst_sel = is_load | is_store;
    tri misc_sel = is_lui | is_auipc;
    tri sys_sel = is_mem | is_system;

    tri branch_done;
    tri ldst_done;

    always_comb begin
        case (opcode)
            OPCODE_LUI: ex_req_slv.rdy = 1'b1;
            OPCODE_AUIPC: ex_req_slv.rdy = 1'b1;
            OPCODE_JAL: ex_req_slv.rdy = branch_done;
            OPCODE_JALR: ex_req_slv.rdy = branch_done;
            OPCODE_BRANCH: ex_req_slv.rdy = branch_done;
            OPCODE_LOAD: ex_req_slv.rdy = ldst_done;
            OPCODE_STORE: ex_req_slv.rdy = ldst_done;
            OPCODE_ALUI: ex_req_slv.rdy = 1'b1;
            OPCODE_ALU: ex_req_slv.rdy = 1'b1;
            OPCODE_MEM: ex_req_slv.rdy = 1'b1;
            OPCODE_SYSTEM: ex_req_slv.rdy = 1'b1;
            default: ex_req_slv.rdy = 1'b0;
        endcase
    end

    logic chn_sels[INST_HANDLER_NUM];
    assign chn_sels[ALU_CHN_IDX] = alu_sel;
    assign chn_sels[BRANCH_CHN_IDX] = branch_sel;
    assign chn_sels[LDST_CHN_IDX] = ldst_sel;
    assign chn_sels[MISC_CHN_IDX] = misc_sel;

    exu_gpr_r_if_t gpr_r1_src_arr[INST_HANDLER_NUM]();
    exu_gpr_r_if_t gpr_r2_src_arr[INST_HANDLER_NUM]();
    exu_gpr_w_if_t gpr_w_src_arr[INST_HANDLER_NUM]();

    exu_gpr_r_if_t gpr_r1_dst();
    exu_gpr_r_if_t gpr_r2_dst();
    exu_gpr_w_if_t gpr_w_dst();

    exu_gpr u_exu_gpr(
        .clk          (clk),
        .gpr_r1_slv   (gpr_r1_dst),
        .gpr_r2_slv   (gpr_r2_dst),
        .gpr_w_slv    (gpr_w_dst)
    );

    exu_gpr_rw_mux #(
        .CHN_NUM      (INST_HANDLER_NUM)
    ) u_exu_gpr_rw_mux(
        .chn_sels     (chn_sels),
        .gpr_r1_slvs  (gpr_r1_src_arr),
        .gpr_r2_slvs  (gpr_r2_src_arr),
        .gpr_w_slvs   (gpr_w_src_arr),
        .gpr_r1_mst   (gpr_r1_dst),
        .gpr_r2_mst   (gpr_r2_dst),
        .gpr_w_mst    (gpr_w_dst)
    );

    exu_alu_handler u_exu_alu_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (alu_sel),
        .inst         (ex_req_slv.pkt.ir),
        .gpr_r1_mst   (gpr_r1_src_arr[ALU_CHN_IDX]),
        .gpr_r2_mst   (gpr_r2_src_arr[ALU_CHN_IDX]),
        .gpr_w_mst    (gpr_w_src_arr[ALU_CHN_IDX])
    );

    exu_branch_handler u_exu_branch_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (branch_sel),
        .inst         (ex_req_slv.pkt.ir),
        .ex_rsp_mst   (ex_rsp_mst),
        .gpr_r1_mst   (gpr_r1_src_arr[BRANCH_CHN_IDX]),
        .gpr_r2_mst   (gpr_r2_src_arr[BRANCH_CHN_IDX]),
        .gpr_w_mst    (gpr_w_src_arr[BRANCH_CHN_IDX]),
        .done         (branch_done)
    );

    exu_ldst_handler u_exu_ldst_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (ldst_sel),
        .inst         (ex_req_slv.pkt.ir),
        .ldst_req_mst (ldst_req_mst),
        .ldst_rsp_slv (ldst_rsp_slv),
        .gpr_r1_mst   (gpr_r1_src_arr[LDST_CHN_IDX]),
        .gpr_r2_mst   (gpr_r2_src_arr[LDST_CHN_IDX]),
        .gpr_w_mst    (gpr_w_src_arr[LDST_CHN_IDX]),
        .done         (ldst_done)
    );

    exu_misc_handler u_exu_misc_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (misc_sel),
        .inst         (ex_req_slv.pkt.ir),
        .gpr_r1_mst   (gpr_r1_src_arr[LDST_CHN_IDX]),
        .gpr_r2_mst   (gpr_r2_src_arr[LDST_CHN_IDX]),
        .gpr_w_mst    (gpr_w_src_arr[LDST_CHN_IDX])
    );

endmodule