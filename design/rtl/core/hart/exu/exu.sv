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
    tlb_flush_if_t.mst tlb_flush_mst,
    l1_flush_if_t.mst l1i_flush_mst,
    l1_flush_if_t.mst l1d_flush_mst,
    l1_flush_ack_if_t.slv l1i_flush_ack_slv,
    l1_flush_ack_if_t.slv l1d_flush_ack_slv,
    hart_expt_if_t.mst ex_expt_mst,
    exu_state_if_t.mst exu_state_mst,
    trap_exu_ctrl_if_t.slv trap_exu_ctrl_slv,
    perf_exu_if_t.mst perf_mst
);
    logic [1:0] priv;
    logic wfi;
    logic [31:0] irq_epc;
    logic irq_defer;

    tri [`RV_OPC_SIZE-1:0] opcode = ex_req_slv.pkt.inst.base.opcode;
    logic [31:0] ex_req_next_pc;
    wire ex_req_hsk = ex_req_slv.vld && ex_req_slv.rdy;
    wire flush_active;
    wire ex_req_fire = ex_req_hsk && !flush_active;
    wire sys_hsk = ex_req_fire && opcode == OPCODE_SYSTEM;
    wire branch_redirect;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            priv <= 2'b11;
            wfi <= 1'b0;
            irq_epc <= 0;
            irq_defer <= 1'b0;
        end else if (trap_exu_ctrl_slv.vld) begin
            priv <= trap_exu_ctrl_slv.pkt.priv;
            wfi <= trap_exu_ctrl_slv.pkt.wfi;
            irq_epc <= trap_exu_ctrl_slv.pkt.irq_epc;
            irq_defer <= 1'b0;
        end else begin
            if (ex_req_fire)
                irq_epc <= ex_req_next_pc;
            irq_defer <= !wfi && !ex_req_slv.rdy;
            if (sys_hsk && ex_req_slv.pkt.inst.i.funct3 == 3'b000 &&
                ex_req_slv.pkt.inst.i.imm_11_0 == 12'h105)
                wfi <= 1'b1;
        end
    end

    assign exu_state_mst.pkt.priv = priv;
    assign exu_state_mst.pkt.pc = ex_req_slv.pkt.pc;
    assign exu_state_mst.pkt.irq_epc = irq_epc;
    assign exu_state_mst.pkt.irq_defer = irq_defer;
    assign exu_state_mst.pkt.wfi = wfi;
    assign exu_state_mst.pkt.wfi_resume_pc = irq_epc;
    localparam INST_HANDLER_NUM = 5;
    localparam ALU_CHN_IDX = 0;
    localparam BRANCH_CHN_IDX = 1;
    localparam LDST_CHN_IDX = 2;
    localparam MISC_CHN_IDX = 3;
    localparam SYS_CHN_IDX = 4;

    logic need_fl;
    assign flush_active = fl_req_slv.vld || need_fl;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            need_fl <= 1'b0;
        else if (fl_req_slv.vld)
            need_fl <= 1'b1;
        else if (need_fl)
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

    tri inst_fire_en = (~flush_active) & (~wfi);
    tri alu_sel = inst_fire_en & (is_alu | is_alui);
    tri branch_sel = inst_fire_en & (is_jal | is_jalr | is_branch);
    tri branch_gpr_sel = branch_sel | ex_rsp_mst.vld;
    tri ldst_sel = inst_fire_en & (is_load | is_store | is_amo);
    tri misc_sel = inst_fire_en & (is_lui | is_auipc);
    tri sys_sel = inst_fire_en & (is_mem | is_sys);

    tri alu_done;
    tri branch_done;
    tri ldst_done;
    logic sys_done;

    logic ex_req_rdy;

    assign branch_redirect = branch_sel && ex_rsp_mst.vld && ex_rsp_mst.rdy &&
        ex_rsp_mst.pkt.taken;
    assign ex_req_next_pc = branch_redirect ? ex_rsp_mst.pkt.target_pc :
        ex_req_slv.pkt.pc + 32'd4;

    assign ex_req_slv.rdy = wfi ? 1'b0 : (flush_active ? 1'b1 : ex_req_rdy);

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
            OPCODE_ALUI: ex_req_rdy = alu_done;
            OPCODE_ALU: ex_req_rdy = alu_done;
            OPCODE_MISC_MEM: ex_req_rdy = sys_done;
            OPCODE_SYSTEM: ex_req_rdy = sys_done;
            default: ex_req_rdy = 1'b0;
        endcase
    end

    logic chn_sels[INST_HANDLER_NUM];
    assign chn_sels[ALU_CHN_IDX] = alu_sel;
    assign chn_sels[BRANCH_CHN_IDX] = branch_gpr_sel;
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
        .gpr_mst      (gpr_src_if_arr[ALU_CHN_IDX]),
        .done         (alu_done)
    );

    exu_branch_handler u_exu_branch_handler(
        .clk          (clk),
        .rst_n        (rst_n),
        .flush        (flush_active),
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
        .tlb_flush_mst     (tlb_flush_mst),
        .l1i_flush_mst     (l1i_flush_mst),
        .l1d_flush_mst     (l1d_flush_mst),
        .l1i_flush_ack_slv (l1i_flush_ack_slv),
        .l1d_flush_ack_slv (l1d_flush_ack_slv),
        .priv              (priv),
        .pc                (ex_req_slv.pkt.pc),
        .inst_hsk          (ex_req_fire),
        .done              (sys_done),
        .ex_expt_mst       (ex_expt_mst)
    );

    assign perf_mst.pkt.ex_req = ex_req_hsk;
    assign perf_mst.pkt.alu_inst = ex_req_fire &&
        (opcode == OPCODE_ALUI || opcode == OPCODE_ALU);
    assign perf_mst.pkt.branch_inst = ex_req_fire &&
        (opcode == OPCODE_JAL || opcode == OPCODE_JALR ||
        opcode == OPCODE_BRANCH);
    assign perf_mst.pkt.ldst_inst = ex_req_fire &&
        (opcode == OPCODE_LOAD || opcode == OPCODE_STORE ||
        opcode == OPCODE_AMO);
    assign perf_mst.pkt.misc_inst = ex_req_fire &&
        (opcode == OPCODE_LUI || opcode == OPCODE_AUIPC);
    assign perf_mst.pkt.sys_inst = ex_req_fire &&
        (opcode == OPCODE_MISC_MEM || opcode == OPCODE_SYSTEM);
    assign perf_mst.pkt.wfi_stall = ex_req_slv.vld && wfi;
    assign perf_mst.pkt.flush_drop = ex_req_slv.vld && flush_active;
    assign perf_mst.pkt.alu_busy = alu_sel && !alu_done;
    assign perf_mst.pkt.branch_busy = branch_sel && !branch_done;
    assign perf_mst.pkt.ldst_busy = ldst_sel && !ldst_done;
    assign perf_mst.pkt.sys_busy = sys_sel && !sys_done;

`ifndef SYNTHESIS
    logic rtl_progress_en;
    longint unsigned rtl_progress_cycle;

    initial begin
        rtl_progress_en = $test$plusargs("rtl_progress");
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rtl_progress_cycle <= 0;
        end else if (rtl_progress_en) begin
            rtl_progress_cycle <= rtl_progress_cycle + 1;
            if (ex_req_hsk && ex_req_slv.pkt.pc >= 32'h10000180 &&
                ex_req_slv.pkt.pc <= 32'h10000260) begin
                $display("[RTL_PROGRESS][%m] cycle=%0d exec pc=%08x inst=%08x opcode=%02x priv=%0d fire=%0b drop=%0b sys=%0b csr_addr=%03x csr_rd_ok=%0b csr_wr=%0b/%0b expt=%0b type=%0d cause=%0d flush=%0b wfi=%0b",
                    rtl_progress_cycle,
                    ex_req_slv.pkt.pc,
                    ex_req_slv.pkt.inst.raw,
                    opcode,
                    priv,
                    ex_req_fire,
                    flush_active,
                    sys_sel,
                    exu_csr_read_req_mst.pkt.addr,
                    csr_exu_read_rsp_slv.pkt.ok,
                    exu_csr_write_req_mst.vld,
                    csr_exu_write_rsp_slv.pkt.ok,
                    ex_expt_mst.vld,
                    ex_expt_mst.pkt.expt_type,
                    ex_expt_mst.pkt.cause,
                    flush_active,
                    wfi);
            end
        end
    end
`endif

endmodule
