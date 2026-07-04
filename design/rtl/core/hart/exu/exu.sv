`include "spec/core/isa.svh"

module exu(
    input logic       clk,
    input logic       rst_n,
    ex_req_if_t.slv   ex_req_slv,
    ex_rsp_if_t.mst   ex_rsp_mst,
    fl_req_if_t.slv   fl_req_slv,
    ldst_req_if_t.mst ldst_req_mst,
    ldst_rsp_if_t.slv ldst_rsp_slv,
    exu_csr_read_req_if_t.mst exu_csr_read_req_mst,
    csr_exu_read_rsp_if_t.slv csr_exu_read_rsp_slv,
    exu_csr_write_req_if_t.mst exu_csr_write_req_mst,
    csr_exu_write_rsp_if_t.slv csr_exu_write_rsp_slv,
    hart_expt_if_t.mst ex_expt_mst,
    exu_state_if_t.mst exu_state_mst,
    trap_exu_ctrl_if_t.slv trap_exu_ctrl_slv
);
    logic [1:0] priv;
    logic wfi;
    logic [31:0] irq_epc;

    tri [`RV_OPC_SIZE-1:0] opcode = ex_req_slv.pkt.inst.base.opcode;
    wire sys_hsk = ex_req_slv.vld && ex_req_slv.rdy && opcode == OPCODE_SYSTEM;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            priv <= 2'b11;
            wfi <= 1'b0;
            irq_epc <= 0;
        end else if (trap_exu_ctrl_slv.vld) begin
            priv <= trap_exu_ctrl_slv.pkt.priv;
            wfi <= trap_exu_ctrl_slv.pkt.wfi;
            irq_epc <= trap_exu_ctrl_slv.pkt.irq_epc;
        end else begin
            if (ex_req_slv.vld && ex_req_slv.rdy)
                irq_epc <= ex_req_slv.pkt.pc + 4;
            if (sys_hsk && ex_req_slv.pkt.inst.i.funct3 == 3'b000 &&
                ex_req_slv.pkt.inst.i.imm_11_0 == 12'h105)
                wfi <= 1'b1;
        end
    end

    assign exu_state_mst.pkt.priv = priv;
    assign exu_state_mst.pkt.pc = ex_req_slv.pkt.pc;
    assign exu_state_mst.pkt.irq_epc = irq_epc;
    assign exu_state_mst.pkt.irq_defer = !wfi && !ex_req_slv.rdy;
    assign exu_state_mst.pkt.wfi = wfi;
    assign exu_state_mst.pkt.wfi_resume_pc = irq_epc;
    localparam INST_HANDLER_NUM = 5;
    localparam ALU_CHN_IDX = 0;
    localparam BRANCH_CHN_IDX = 1;
    localparam LDST_CHN_IDX = 2;
    localparam MISC_CHN_IDX = 3;
    localparam SYS_CHN_IDX = 4;

    logic need_fl;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            need_fl <= 1'b0;
        else if (fl_req_slv.vld)
            need_fl <= 1'b1;
        else if (ex_req_slv.vld)
            need_fl <= 1'b0;
    end

    tri is_lui = ex_req_slv.vld & (opcode == OPCODE_LUI);
    tri is_auipc = ex_req_slv.vld & (opcode == OPCODE_AUIPC);
    tri is_jal = ex_req_slv.vld & (opcode == OPCODE_JAL);
    tri is_jalr = ex_req_slv.vld & (opcode == OPCODE_JALR);
    tri is_branch = ex_req_slv.vld & (opcode == OPCODE_BRANCH);
    tri is_load = ex_req_slv.vld & (opcode == OPCODE_LOAD);
    tri is_store = ex_req_slv.vld & (opcode == OPCODE_STORE);
    tri is_amo = ex_req_slv.vld & (opcode == OPCODE_AMO);
    tri is_alui = ex_req_slv.vld & (opcode == OPCODE_ALUI);
    tri is_alu = ex_req_slv.vld & (opcode == OPCODE_ALU);
    tri is_mem = ex_req_slv.vld & (opcode == OPCODE_MISC_MEM);
    tri is_sys = ex_req_slv.vld & (opcode == OPCODE_SYSTEM);

    tri alu_sel = (~need_fl) & (is_alu | is_alui);
    tri branch_sel = (~need_fl) & (is_jal | is_jalr | is_branch);
    tri ldst_sel = (~need_fl) & (is_load | is_store | is_amo);
    tri misc_sel = (~need_fl) & (is_lui | is_auipc);
    tri sys_sel = (~need_fl) & (is_mem | is_sys);

    tri branch_done;
    tri ldst_done;

    logic ex_req_rdy;
    assign ex_req_slv.rdy = wfi ? 1'b0 : (need_fl ? 1'b1 : ex_req_rdy);

    always_comb begin
        case (opcode)
            OPCODE_LUI: ex_req_rdy = 1'b1;
            OPCODE_AUIPC: ex_req_rdy = 1'b1;
            OPCODE_JAL: ex_req_rdy = branch_done;
            OPCODE_JALR: ex_req_rdy = branch_done;
            OPCODE_BRANCH: ex_req_rdy = branch_done;
            OPCODE_LOAD: ex_req_rdy = ldst_done;
            OPCODE_STORE: ex_req_rdy = ldst_done;
            OPCODE_AMO: ex_req_rdy = ldst_done;
            OPCODE_ALUI: ex_req_rdy = 1'b1;
            OPCODE_ALU: ex_req_rdy = 1'b1;
            OPCODE_MISC_MEM: ex_req_rdy = 1'b1;
            OPCODE_SYSTEM: ex_req_rdy = 1'b1;
            default: ex_req_rdy = 1'b0;
        endcase
    end

    logic chn_sels[INST_HANDLER_NUM];
    assign chn_sels[ALU_CHN_IDX] = alu_sel;
    assign chn_sels[BRANCH_CHN_IDX] = branch_sel;
    assign chn_sels[LDST_CHN_IDX] = ldst_sel;
    assign chn_sels[MISC_CHN_IDX] = misc_sel;
    assign chn_sels[SYS_CHN_IDX] = sys_sel;

    exu_gpr_if_t gpr_src_if_arr[INST_HANDLER_NUM]();
    exu_gpr_if_t gpr_dst_if();

    exu_gpr u_exu_gpr(
        .clk          (clk),
        .gpr_slv      (gpr_dst_if)
    );

    exu_gpr_rw_mux #(
        .CHN_NUM      (INST_HANDLER_NUM)
    ) u_exu_gpr_rw_mux(
        .chn_sels     (chn_sels),
        .gpr_src_slvs (gpr_src_if_arr),
        .gpr_dst_mst  (gpr_dst_if)
    );

    exu_alu_handler u_exu_alu_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (alu_sel),
        .inst         (ex_req_slv.pkt.inst),
        .gpr_mst      (gpr_src_if_arr[ALU_CHN_IDX])
    );

    exu_branch_handler u_exu_branch_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (branch_sel),
        .inst         (ex_req_slv.pkt.inst),
        .pc           (ex_req_slv.pkt.pc),
        .pred_taken   (ex_req_slv.pkt.pred_taken),
        .pred_pc      (ex_req_slv.pkt.pred_pc),
        .ex_rsp_mst   (ex_rsp_mst),
        .gpr_mst      (gpr_src_if_arr[BRANCH_CHN_IDX]),
        .done         (branch_done)
    );

    exu_ldst_handler u_exu_ldst_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (ldst_sel),
        .inst         (ex_req_slv.pkt.inst),
        .ldst_req_mst (ldst_req_mst),
        .ldst_rsp_slv (ldst_rsp_slv),
        .gpr_mst      (gpr_src_if_arr[LDST_CHN_IDX]),
        .done         (ldst_done)
    );

    exu_misc_handler u_exu_misc_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (misc_sel),
        .inst         (ex_req_slv.pkt.inst),
        .pc           (ex_req_slv.pkt.pc),
        .gpr_mst      (gpr_src_if_arr[MISC_CHN_IDX])
    );

    exu_sys_handler u_exu_sys_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .sel          (sys_sel),
        .inst         (ex_req_slv.pkt.inst),
        .gpr_mst      (gpr_src_if_arr[SYS_CHN_IDX]),
        .csr_read_req_mst  (exu_csr_read_req_mst),
        .csr_read_rsp_slv  (csr_exu_read_rsp_slv),
        .csr_write_req_mst (exu_csr_write_req_mst),
        .csr_write_rsp_slv (csr_exu_write_rsp_slv),
        .priv              (priv),
        .pc                (ex_req_slv.pkt.pc),
        .ex_expt_mst       (ex_expt_mst)
    );

endmodule
