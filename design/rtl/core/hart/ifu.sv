`include "spec/core/isa.svh"

module ifu(
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
    trap_send_if_t.slv trap_send_slv
);
    typedef enum logic [2:0] {
        FETCH_REQ,
        FETCH_RESP,
        PRED,
        EXEC,
        FETCH_DROP
    } state_t;

    state_t state;
    logic [`RV_PC_SIZE-1:0] pc;
    logic [`RV_PC_SIZE-1:0] req_pc;
    logic [`RV_IR_SIZE-1:0] ir_raw;
    logic pred_is_ctrl;
    logic pred_taken;
    logic [`RV_PC_SIZE-1:0] pred_pc;
    logic pred_cond_bht_hit;
    logic pred_jalr_ras_hit;
    logic pred_jalr_btb_hit;
    logic pred_jalr_btb_miss;

    wire fch_req_hsk = fch_req_mst.vld && fch_req_mst.rdy;
    wire fch_rsp_hsk = fch_rsp_slv.vld && fch_rsp_slv.rdy;
    wire ex_req_hsk = ex_req_mst.vld && ex_req_mst.rdy;
    wire ex_rsp_hsk = ex_rsp_slv.vld && ex_rsp_slv.rdy;
    wire pred_rsp_vld = bpu_pred_rsp_slv.pkt.vld;
    wire redirect = ex_rsp_hsk && pred_is_ctrl && !ex_rsp_slv.pkt.pred_true;
    wire frontend_flush = tlb_flush_slv.vld || l1i_flush_vld;
    wire [`RV_PC_SIZE-1:0] pred_next_pc =
        pred_taken ? pred_pc : req_pc + 32'd4;
    wire [`RV_PC_SIZE-1:0] next_pc =
        redirect ? ex_rsp_slv.pkt.target_pc : pred_next_pc;
    wire is_boot_code = req_pc < 32'h00000800;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= FETCH_REQ;
            pc <= 32'h00000000;
            req_pc <= '0;
            ir_raw <= '0;
            pred_is_ctrl <= 1'b0;
            pred_taken <= 1'b0;
            pred_pc <= '0;
            pred_cond_bht_hit <= 1'b0;
            pred_jalr_ras_hit <= 1'b0;
            pred_jalr_btb_hit <= 1'b0;
            pred_jalr_btb_miss <= 1'b0;
        end else if (trap_send_slv.vld) begin
            pc <= trap_send_slv.pkt.target_pc;
            pred_is_ctrl <= 1'b0;
            pred_taken <= 1'b0;
            if ((state == FETCH_REQ && fch_req_hsk) ||
                ((state == FETCH_RESP || state == FETCH_DROP) && !fch_rsp_hsk))
                state <= FETCH_DROP;
            else
                state <= FETCH_REQ;
        end else if (frontend_flush) begin
            pc <= state == EXEC ? req_pc + 32'd4 : pc;
            pred_is_ctrl <= 1'b0;
            pred_taken <= 1'b0;
            if ((state == FETCH_REQ && fch_req_hsk) ||
                ((state == FETCH_RESP || state == FETCH_DROP) && !fch_rsp_hsk))
                state <= FETCH_DROP;
            else
                state <= FETCH_REQ;
        end else begin
            case (state)
                FETCH_REQ: begin
                    if (fch_req_hsk) begin
                        req_pc <= pc;
                        state <= FETCH_RESP;
                    end
                end
                FETCH_RESP: begin
                    if (fch_rsp_hsk) begin
                        ir_raw <= fch_rsp_slv.pkt.ir;
                        state <= PRED;
                    end
                end
                PRED: begin
                    if (pred_rsp_vld) begin
                        pred_is_ctrl <= bpu_pred_rsp_slv.pkt.is_ctrl;
                        pred_taken <= bpu_pred_rsp_slv.pkt.pred_taken;
                        pred_pc <= bpu_pred_rsp_slv.pkt.pred_pc;
                        pred_cond_bht_hit <= bpu_pred_rsp_slv.pkt.cond_bht_hit;
                        pred_jalr_ras_hit <= bpu_pred_rsp_slv.pkt.jalr_ras_hit;
                        pred_jalr_btb_hit <= bpu_pred_rsp_slv.pkt.jalr_btb_hit;
                        pred_jalr_btb_miss <= bpu_pred_rsp_slv.pkt.jalr_btb_miss;
                        state <= EXEC;
                    end
                end
                EXEC: begin
                    if (ex_req_hsk) begin
                        pc <= next_pc;
                        pred_is_ctrl <= 1'b0;
                        pred_taken <= 1'b0;
                        state <= FETCH_REQ;
                    end
                end
                FETCH_DROP: begin
                    if (fch_rsp_hsk)
                        state <= FETCH_REQ;
                end
                default: state <= FETCH_REQ;
            endcase
        end
    end

    assign fch_req_mst.vld = state == FETCH_REQ;
    assign fch_req_mst.pkt.pc = pc;
    assign fch_rsp_slv.rdy = state == FETCH_RESP || state == FETCH_DROP;

    assign bpu_pred_req_mst.pkt.vld = state == PRED;
    assign bpu_pred_req_mst.pkt.pc = req_pc;
    assign bpu_pred_req_mst.pkt.ir = ir_raw;

    assign bpu_update_mst.pkt.vld = ex_rsp_hsk && pred_is_ctrl;
    assign bpu_update_mst.pkt.pc = req_pc;
    assign bpu_update_mst.pkt.ir = ir_raw;
    assign bpu_update_mst.pkt.taken = ex_rsp_slv.pkt.taken;
    assign bpu_update_mst.pkt.target_pc = ex_rsp_slv.pkt.target_pc;
    assign bpu_update_mst.pkt.pred_taken = pred_taken;
    assign bpu_update_mst.pkt.pred_pc = pred_pc;
    assign bpu_update_mst.pkt.pred_true = ex_rsp_slv.pkt.pred_true;
    assign bpu_update_mst.pkt.is_boot_code = is_boot_code;
    assign bpu_update_mst.pkt.pred_cond_bht_hit = pred_cond_bht_hit;
    assign bpu_update_mst.pkt.pred_jalr_ras_hit = pred_jalr_ras_hit;
    assign bpu_update_mst.pkt.pred_jalr_btb_hit = pred_jalr_btb_hit;
    assign bpu_update_mst.pkt.pred_jalr_btb_miss = pred_jalr_btb_miss;

    assign ex_req_mst.vld = state == EXEC;
    assign ex_req_mst.pkt.inst.raw = ir_raw;
    assign ex_req_mst.pkt.pc = req_pc;
    assign ex_req_mst.pkt.pred_taken = pred_taken;
    assign ex_req_mst.pkt.pred_pc = pred_pc;
    assign ex_req_mst.pkt.is_boot_code = is_boot_code;
    assign ex_rsp_slv.rdy = state == EXEC;

    assign fl_req_mst.vld = 1'b0;
endmodule
