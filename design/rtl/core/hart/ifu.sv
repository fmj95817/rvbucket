`include "spec/core/isa.svh"
`include "spec/core/hart.svh"
`include "itf/hart_expt_if.svh"

module ifu #(
    parameter int CTRLQ_DEPTH = `HART_IFU_CTRLQ_DEPTH,
    parameter int FCH_OST_DEPTH = `HART_IFU_FCH_OST_DEPTH
)(
    input logic      clk,
    input logic      rst_n,
    fch_req_if_t.mst fch_req_mst,
    fch_rsp_if_t.slv fch_rsp_slv,
    ex_req_if_t.mst  ex_req_mst,
    ex_rsp_if_t.slv  ex_rsp_slv,
    fl_req_if_t.mst  fl_req_mst,
    bpu_pred_req_if_t.mst bpu_pred_req_mst,
    bpu_pred_rsp_if_t.slv bpu_pred_rsp_slv,
    bpu_update_if_t.mst   bpu_update_mst,
    tlb_flush_if_t.slv tlb_flush_slv,
    input logic l1i_flush_vld,
    hart_expt_if_t.mst fch_expt_mst,
    trap_send_if_t.slv trap_send_slv,
    exu_state_if_t.slv exu_state_slv
);
    localparam int FCH_OST_SLOT_W = $clog2(FCH_OST_DEPTH);
    localparam int FCH_EPOCH_W = FCH_OST_SLOT_W + 1;

    typedef struct packed {
        logic [31:0] pc;
        logic [FCH_EPOCH_W-1:0] epoch;
    } fch_ost_ctx_t;

    typedef struct packed {
        logic [31:0] pc;
        logic [31:0] ir;
        logic ok;
        logic expt;
        logic [31:0] cause;
        logic [1:0] priv;
        logic [31:0] tval;
    } fch_rspq_entry_t;

    typedef struct packed {
        logic [31:0] pc;
        logic [31:0] ir;
        logic pred_taken;
        logic [31:0] pred_pc;
        logic cond_bht_hit;
        logic jalr_ras_hit;
        logic jalr_btb_hit;
        logic jalr_btb_miss;
    } ctrlq_entry_t;

    localparam int FCH_OST_CTX_DW = $bits(fch_ost_ctx_t);
    localparam int FCH_RSPQ_DW = $bits(fch_rspq_entry_t);
    localparam int CTRLQ_DW = $bits(ctrlq_entry_t);

    logic [31:0] fch_pc;
    logic [FCH_EPOCH_W-1:0] fch_epoch;
    logic trap_pend_vld;
    logic [31:0] trap_pend_pc;

    logic fch_ost_alloc_rdy;
    logic [FCH_OST_SLOT_W-1:0] fch_ost_alloc_slot;
    logic fch_ost_head_vld;
    logic [FCH_OST_CTX_DW-1:0] fch_ost_head_ctx_raw;
    logic [FCH_OST_SLOT_W-1:0] fch_ost_head_slot;
    fch_ost_ctx_t fch_ost_head_ctx;
    logic fch_ost_free_head;

    logic fch_rspq_wr_vld;
    logic fch_rspq_wr_rdy;
    logic [FCH_RSPQ_DW-1:0] fch_rspq_wr_data;
    logic fch_rspq_rd_vld;
    logic fch_rspq_rd_rdy;
    logic [FCH_RSPQ_DW-1:0] fch_rspq_rd_data;
    logic fch_rspq_clear;
    fch_rspq_entry_t fch_rspq_head;
    fch_rspq_entry_t fch_rspq_push_entry;

    logic ctrlq_wr_vld;
    logic ctrlq_wr_rdy;
    logic [CTRLQ_DW-1:0] ctrlq_wr_data;
    logic ctrlq_rd_vld;
    logic ctrlq_rd_rdy;
    logic [CTRLQ_DW-1:0] ctrlq_rd_data;
    logic ctrlq_clear;
    ctrlq_entry_t ctrlq_head;
    ctrlq_entry_t ctrlq_push_entry;
    ctrlq_entry_t br_rsp_ctrlq;
    logic br_rsp_vld;
    logic br_rsp_taken;
    logic br_rsp_pred_true;
    logic [31:0] br_rsp_target_pc;

    logic issue_vld;
    logic issue_ctrlq_pushed;
    logic [31:0] issue_pc;
    logic [31:0] issue_ir;
    logic issue_is_ctrl;
    logic issue_pred_taken;
    logic [31:0] issue_pred_pc;
    logic issue_cond_bht_hit;
    logic issue_jalr_ras_hit;
    logic issue_jalr_btb_hit;
    logic issue_jalr_btb_miss;

    rv32g_inst_t issue_inst;
    rv32g_inst_t rspq_inst;

    wire fch_req_hsk = fch_req_mst.vld && fch_req_mst.rdy;
    wire fch_rsp_hsk = fch_rsp_slv.vld && fch_rsp_slv.rdy;
    wire ex_req_hsk = ex_req_mst.vld && ex_req_mst.rdy;
    wire ex_rsp_hsk = ex_rsp_slv.vld && ex_rsp_slv.rdy;
    wire ctrlq_wr_hsk = ctrlq_wr_vld && ctrlq_wr_rdy;

    wire trap_req_vld = trap_pend_vld || trap_send_slv.vld;
    wire [31:0] trap_req_pc = trap_pend_vld ? trap_pend_pc :
        trap_send_slv.pkt.target_pc;
    wire frontend_flush = tlb_flush_slv.vld || l1i_flush_vld;
    wire branch_redirect = br_rsp_vld && !br_rsp_pred_true;
    wire trap_redirect = trap_req_vld;
    wire control_redirect = branch_redirect || trap_redirect;
    wire redirect_flush = control_redirect || frontend_flush;
    wire [31:0] redirect_pc = trap_redirect ? trap_req_pc :
        (branch_redirect ? br_rsp_target_pc :
        exu_state_slv.pkt.irq_epc);
    wire rspq_expt = fch_rspq_rd_vld && fch_rspq_head.expt;
    wire rspq_non_expt_bad = fch_rspq_rd_vld && !fch_rspq_head.ok &&
        !fch_rspq_head.expt;
    wire rspq_expt_fire = rspq_expt && !ctrlq_rd_vld && !control_redirect &&
        !frontend_flush;
    wire rspq_any_bad = fch_rspq_rd_vld &&
        (!fch_rspq_head.ok || fch_rspq_head.expt);
    wire rspq_bad = rspq_non_expt_bad || rspq_expt_fire;
    wire pred_req_vld = fch_rspq_rd_vld && !issue_vld && !ctrlq_rd_vld &&
        !trap_req_vld &&
        !rspq_any_bad && !redirect_flush;
    wire pred_rsp_vld = bpu_pred_rsp_slv.pkt.vld;
    wire prepare_issue = pred_req_vld && pred_rsp_vld;
    wire pred_taken_flush = prepare_issue && bpu_pred_rsp_slv.pkt.is_ctrl &&
        bpu_pred_rsp_slv.pkt.pred_taken;
    wire [31:0] pred_next_pc = bpu_pred_rsp_slv.pkt.pred_pc;
    wire is_boot_code = issue_pc < 32'h00000800;

    assign fch_ost_head_ctx = fch_ost_head_ctx_raw;
    assign fch_rspq_head = fch_rspq_rd_data;
    assign ctrlq_head = ctrlq_rd_data;
    assign issue_inst.raw = issue_ir;
    assign rspq_inst.raw = fch_rspq_head.ir;

    function automatic logic inst_is_ctrl(input rv32g_inst_t inst);
        inst_is_ctrl = inst.base.opcode == OPCODE_JAL ||
            inst.base.opcode == OPCODE_JALR ||
            inst.base.opcode == OPCODE_BRANCH;
    endfunction

    ostq #(
        .DW    (FCH_OST_CTX_DW),
        .DEPTH (FCH_OST_DEPTH)
    ) u_fch_ost(
        .clk           (clk),
        .rst_n         (rst_n),
        .alloc_vld     (fch_req_hsk),
        .alloc_rdy     (fch_ost_alloc_rdy),
        .alloc_ctx     (fch_ost_ctx_t'{pc:fch_pc, epoch:fch_epoch}),
        .alloc_slot    (fch_ost_alloc_slot),
        .head_vld      (fch_ost_head_vld),
        .head_ctx      (fch_ost_head_ctx_raw),
        .head_slot     (fch_ost_head_slot),
        .free_head     (fch_ost_free_head),
        .slot_wr_vld   (1'b0),
        .slot_wr_idx   ('0),
        .slot_wr_ctx   ('0),
        .slot_vld      (),
        .slot_ctx_flat (),
        .empty         (),
        .full          ()
    );

    fifo #(
        .DW    (FCH_RSPQ_DW),
        .DEPTH (FCH_OST_DEPTH)
    ) u_fch_rspq(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (fch_rspq_clear),
        .wr_vld  (fch_rspq_wr_vld),
        .wr_rdy  (fch_rspq_wr_rdy),
        .wr_data (fch_rspq_wr_data),
        .rd_vld  (fch_rspq_rd_vld),
        .rd_rdy  (fch_rspq_rd_rdy),
        .rd_data (fch_rspq_rd_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (CTRLQ_DW),
        .DEPTH (CTRLQ_DEPTH)
    ) u_ctrlq(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (ctrlq_clear),
        .wr_vld  (ctrlq_wr_vld),
        .wr_rdy  (ctrlq_wr_rdy),
        .wr_data (ctrlq_wr_data),
        .rd_vld  (ctrlq_rd_vld),
        .rd_rdy  (ctrlq_rd_rdy),
        .rd_data (ctrlq_rd_data),
        .empty   (),
        .full    ()
    );

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && fch_rsp_slv.vld) begin
            assert (fch_ost_head_vld)
                else $fatal(1, "IFU fetch response without OST entry");
        end
        if (rst_n && ex_rsp_slv.vld) begin
            assert (ctrlq_rd_vld)
                else $fatal(1,
                    "IFU branch response without ctrlq entry pc=%08x rsp_pc=%08x issue=%0b issue_pc=%08x ctrlpush=%0b trap=%0b pend=%0b redir=%0b clear=%0b exreq=%0b/%0b",
                    fch_pc, ex_rsp_slv.pkt.pc, issue_vld, issue_pc,
                    issue_ctrlq_pushed, trap_req_vld, trap_pend_vld,
                    redirect_flush, ctrlq_clear, ex_req_mst.vld,
                    ex_req_mst.rdy);
            assert (!ctrlq_rd_vld || ctrlq_head.pc == ex_rsp_slv.pkt.pc)
                else $fatal(1,
                    "IFU branch response pc mismatch ctrlq_pc=%08x rsp_pc=%08x",
                    ctrlq_head.pc, ex_rsp_slv.pkt.pc);
        end
    end
`endif

    always_comb begin
        fch_rspq_push_entry.pc = fch_ost_head_ctx.pc;
        fch_rspq_push_entry.ir = fch_rsp_slv.pkt.ir;
        fch_rspq_push_entry.ok = fch_rsp_slv.pkt.ok;
        fch_rspq_push_entry.expt = fch_rsp_slv.pkt.expt;
        fch_rspq_push_entry.cause = fch_rsp_slv.pkt.cause;
        fch_rspq_push_entry.priv = fch_rsp_slv.pkt.priv;
        fch_rspq_push_entry.tval = fch_rsp_slv.pkt.tval;
    end

    assign fch_rspq_wr_vld = fch_rsp_slv.vld && fch_ost_head_vld &&
        fch_ost_head_ctx.epoch == fch_epoch && !redirect_flush;
    assign fch_rspq_wr_data = fch_rspq_push_entry;
    assign fch_rsp_slv.rdy = fch_ost_head_vld &&
        (fch_ost_head_ctx.epoch != fch_epoch || redirect_flush ||
        fch_rspq_wr_rdy);
    assign fch_ost_free_head = fch_rsp_hsk;

    assign fch_rspq_clear = redirect_flush || pred_taken_flush || rspq_bad;
    assign ctrlq_clear = redirect_flush;
    assign fch_rspq_rd_rdy = prepare_issue || rspq_bad;

    assign fch_expt_mst.vld = rspq_expt_fire;
    assign fch_expt_mst.pkt.expt_type = HART_EXPT_TYPE_EXCEPTION;
    assign fch_expt_mst.pkt.cause = hart_expt_cause_t'(fch_rspq_head.cause);
    assign fch_expt_mst.pkt.priv = fch_rspq_head.priv;
    assign fch_expt_mst.pkt.pc = fch_rspq_head.pc;
    assign fch_expt_mst.pkt.tval = fch_rspq_head.tval;

    always_comb begin
        ctrlq_push_entry.pc = issue_pc;
        ctrlq_push_entry.ir = issue_ir;
        ctrlq_push_entry.pred_taken = issue_pred_taken;
        ctrlq_push_entry.pred_pc = issue_pred_pc;
        ctrlq_push_entry.cond_bht_hit = issue_cond_bht_hit;
        ctrlq_push_entry.jalr_ras_hit = issue_jalr_ras_hit;
        ctrlq_push_entry.jalr_btb_hit = issue_jalr_btb_hit;
        ctrlq_push_entry.jalr_btb_miss = issue_jalr_btb_miss;
    end

    assign ctrlq_wr_vld = issue_vld && issue_is_ctrl &&
        !issue_ctrlq_pushed && !trap_req_vld && !control_redirect;
    assign ctrlq_wr_data = ctrlq_push_entry;
    assign ctrlq_rd_rdy = ex_rsp_hsk;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            fch_pc <= 32'h00000000;
            fch_epoch <= '0;
            trap_pend_vld <= 1'b0;
            trap_pend_pc <= 32'h00000000;
            issue_vld <= 1'b0;
            issue_ctrlq_pushed <= 1'b0;
            issue_pc <= 32'h00000000;
            issue_ir <= 32'h00000000;
            issue_is_ctrl <= 1'b0;
            issue_pred_taken <= 1'b0;
            issue_pred_pc <= 32'h00000004;
            issue_cond_bht_hit <= 1'b0;
            issue_jalr_ras_hit <= 1'b0;
            issue_jalr_btb_hit <= 1'b0;
            issue_jalr_btb_miss <= 1'b0;
            br_rsp_vld <= 1'b0;
            br_rsp_ctrlq <= '0;
            br_rsp_taken <= 1'b0;
            br_rsp_pred_true <= 1'b0;
            br_rsp_target_pc <= 32'h00000000;
        end else begin
            br_rsp_vld <= 1'b0;
            if (ex_rsp_hsk) begin
                br_rsp_vld <= 1'b1;
                br_rsp_ctrlq <= ctrlq_head;
                br_rsp_taken <= ex_rsp_slv.pkt.taken;
                br_rsp_pred_true <= ex_rsp_slv.pkt.pred_true;
                br_rsp_target_pc <= ex_rsp_slv.pkt.target_pc;
            end

            if (trap_redirect) begin
                trap_pend_vld <= 1'b0;
            end else if (trap_send_slv.vld) begin
                trap_pend_vld <= 1'b1;
                trap_pend_pc <= trap_send_slv.pkt.target_pc;
            end

            if (redirect_flush) begin
                fch_pc <= redirect_pc;
                fch_epoch <= fch_epoch + 1'b1;
                issue_vld <= 1'b0;
                issue_ctrlq_pushed <= 1'b0;
                issue_is_ctrl <= 1'b0;
                issue_pred_taken <= 1'b0;
            end else if (rspq_bad) begin
                fch_pc <= fch_rspq_head.pc;
                fch_epoch <= fch_epoch + 1'b1;
                issue_vld <= 1'b0;
                issue_ctrlq_pushed <= 1'b0;
                issue_is_ctrl <= 1'b0;
                issue_pred_taken <= 1'b0;
            end else begin
                if (fch_req_hsk)
                    fch_pc <= fch_pc + 32'd4;

                if (prepare_issue) begin
                    issue_vld <= 1'b1;
                    issue_ctrlq_pushed <= 1'b0;
                    issue_pc <= fch_rspq_head.pc;
                    issue_ir <= fch_rspq_head.ir;
                    issue_is_ctrl <= bpu_pred_rsp_slv.pkt.is_ctrl;
                    issue_pred_taken <= bpu_pred_rsp_slv.pkt.pred_taken;
                    issue_pred_pc <= bpu_pred_rsp_slv.pkt.pred_pc;
                    issue_cond_bht_hit <= bpu_pred_rsp_slv.pkt.cond_bht_hit;
                    issue_jalr_ras_hit <= bpu_pred_rsp_slv.pkt.jalr_ras_hit;
                    issue_jalr_btb_hit <= bpu_pred_rsp_slv.pkt.jalr_btb_hit;
                    issue_jalr_btb_miss <= bpu_pred_rsp_slv.pkt.jalr_btb_miss;
                    if (pred_taken_flush) begin
                        fch_pc <= pred_next_pc;
                        fch_epoch <= fch_epoch + 1'b1;
                    end
                end else if (ctrlq_wr_hsk) begin
                    issue_ctrlq_pushed <= 1'b1;
                end

                if (ex_req_hsk) begin
                    issue_vld <= 1'b0;
                    issue_ctrlq_pushed <= 1'b0;
                    issue_is_ctrl <= 1'b0;
                    issue_pred_taken <= 1'b0;
                end
            end
        end
    end

    assign fch_req_mst.vld = !trap_req_vld && !redirect_flush &&
        !pred_taken_flush && fch_ost_alloc_rdy;
    assign fch_req_mst.pkt.pc = fch_pc;

    assign bpu_pred_req_mst.pkt.vld = pred_req_vld;
    assign bpu_pred_req_mst.pkt.pc = fch_rspq_head.pc;
    assign bpu_pred_req_mst.pkt.ir = fch_rspq_head.ir;

    assign ex_req_mst.vld = issue_vld &&
        (!issue_is_ctrl || issue_ctrlq_pushed) &&
        !trap_req_vld && !control_redirect;
    assign ex_req_mst.pkt.inst.raw = issue_ir;
    assign ex_req_mst.pkt.pc = issue_pc;
    assign ex_req_mst.pkt.pred_taken = issue_pred_taken;
    assign ex_req_mst.pkt.pred_pc = issue_pred_pc;
    assign ex_req_mst.pkt.is_boot_code = is_boot_code;

    assign ex_rsp_slv.rdy = ctrlq_rd_vld && !br_rsp_vld;

    assign bpu_update_mst.pkt.vld = br_rsp_vld;
    assign bpu_update_mst.pkt.pc = br_rsp_ctrlq.pc;
    assign bpu_update_mst.pkt.ir = br_rsp_ctrlq.ir;
    assign bpu_update_mst.pkt.taken = br_rsp_taken;
    assign bpu_update_mst.pkt.target_pc = br_rsp_target_pc;
    assign bpu_update_mst.pkt.pred_taken = br_rsp_ctrlq.pred_taken;
    assign bpu_update_mst.pkt.pred_pc = br_rsp_ctrlq.pred_pc;
    assign bpu_update_mst.pkt.pred_true = br_rsp_pred_true;
    assign bpu_update_mst.pkt.is_boot_code = br_rsp_ctrlq.pc < 32'h00000800;
    assign bpu_update_mst.pkt.pred_cond_bht_hit = br_rsp_ctrlq.cond_bht_hit;
    assign bpu_update_mst.pkt.pred_jalr_ras_hit = br_rsp_ctrlq.jalr_ras_hit;
    assign bpu_update_mst.pkt.pred_jalr_btb_hit = br_rsp_ctrlq.jalr_btb_hit;
    assign bpu_update_mst.pkt.pred_jalr_btb_miss = br_rsp_ctrlq.jalr_btb_miss;

    assign fl_req_mst.vld = branch_redirect || trap_redirect;

    wire unused_fch_ost = (|fch_ost_alloc_slot) | (|fch_ost_head_slot) |
        inst_is_ctrl(issue_inst) | inst_is_ctrl(rspq_inst);

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
            if (rtl_progress_cycle[19:0] == 20'h0 || fch_expt_mst.vld ||
                redirect_flush || rspq_bad) begin
                $display("[RTL_PROGRESS][%m] cycle=%0d pc=%08x epoch=%08x trap=%0b pend=%0b pend_pc=%08x ost_alloc=%0b head=%0b rspq=%0b/%0b ctrlq=%0b/%0b issue=%0b ctrlpush=%0b req=%0b/%0b rsp=%0b/%0b exreq=%0b/%0b exrsp=%0b/%0b bad=%0b expt=%0b expt_fire=%0b redir=%0b trap_redir=%0b br=%0b predflush=%0b",
                    rtl_progress_cycle,
                    fch_pc,
                    fch_epoch,
                    trap_req_vld,
                    trap_pend_vld,
                    trap_pend_pc,
                    fch_ost_alloc_rdy,
                    fch_ost_head_vld,
                    fch_rspq_rd_vld,
                    fch_rspq_rd_rdy,
                    ctrlq_rd_vld,
                    ctrlq_rd_rdy,
                    issue_vld,
                    issue_ctrlq_pushed,
                    fch_req_mst.vld,
                    fch_req_mst.rdy,
                    fch_rsp_slv.vld,
                    fch_rsp_slv.rdy,
                    ex_req_mst.vld,
                    ex_req_mst.rdy,
                    ex_rsp_slv.vld,
                    ex_rsp_slv.rdy,
                    rspq_bad,
                    rspq_expt,
                    rspq_expt_fire,
                    redirect_flush,
                    trap_redirect,
                    branch_redirect,
                    pred_taken_flush);
            end
        end
    end
`endif
endmodule
