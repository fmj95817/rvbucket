`include "isa.svh"

module ifu(
    input logic      clk,
    input logic      rst_n,
    fch_req_if_t.mst fch_req_mst,
    fch_rsp_if_t.slv fch_rsp_slv,
    ex_req_if_t.mst  ex_req_mst,
    ex_rsp_if_t.slv  ex_rsp_slv,
    fl_req_if_t.mst  fl_req_mst
);
    tri fch_req_hsk = fch_req_mst.vld & fch_req_mst.rdy;
    tri fch_rsp_hsk = fch_rsp_slv.vld & fch_rsp_slv.rdy;
    tri ex_req_hsk = ex_req_mst.vld & ex_req_mst.rdy;
    tri ex_rsp_hsk = ex_rsp_slv.vld & ex_rsp_slv.rdy;

    logic ir_valid_flag;
    logic fch_req_pend_flag;
    logic ex_req_pkt_valid;
    logic pc_pend_flag;

    tri ir_valid_set = fch_rsp_hsk;
    tri ir_valid_clear = ex_req_hsk;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir_valid_flag <= 1'b0;
        else if (ir_valid_set)
            ir_valid_flag <= 1'b1;
        else if (ir_valid_clear)
            ir_valid_flag <= 1'b0;
    end

    tri fch_req_pend_set = fch_req_hsk;
    tri fch_req_pend_clear = fch_rsp_hsk;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            fch_req_pend_flag <= 1'b0;
        else if (fch_req_pend_set)
            fch_req_pend_flag <= 1'b1;
        else if (fch_req_pend_clear)
            fch_req_pend_flag <= 1'b0;
    end

    tri ex_req_pkt_valid_set = pc_pend_flag & fch_rsp_slv.rdy;
    tri ex_req_pkt_valid_clear = ex_req_hsk;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ex_req_pkt_valid <= 1'b0;
        else if (ex_req_pkt_valid_set)
            ex_req_pkt_valid <= 1'b1;
        else if (ex_req_pkt_valid_clear)
            ex_req_pkt_valid <= 1'b0;
    end

    tri pc_pend_set = fch_req_hsk;
    tri pc_pend_clear = ex_req_pkt_valid_set;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_pend_flag <= 1'b0;
        else if (pc_pend_set)
            pc_pend_flag <= 1'b1;
        else if (pc_pend_clear)
            pc_pend_flag <= 1'b0;
    end

    logic [`RV_IR_SIZE-1:0] ir;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir <= {`RV_IR_SIZE{1'b0}};
        else if (ir_valid_set)
            ir <= fch_rsp_slv.pkt.ir;
    end

    logic [`RV_PC_SIZE-1:0] pc_offset;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_offset <= {`RV_PC_SIZE{1'b0}};
        else
            pc_offset <= `RV_PC_SIZE'd4;
    end

    logic [`RV_PC_SIZE-1:0] pc;
    tri taken = ex_rsp_hsk & ex_rsp_slv.pkt.taken;
    tri [`RV_PC_SIZE-1:0] pc_not_tk_nxt = pc + pc_offset;
    tri [`RV_PC_SIZE-1:0] pc_nxt = taken ? ex_rsp_slv.pkt.target_pc : pc_not_tk_nxt;

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc <= {`RV_PC_SIZE{1'b0}};
        else if (fch_req_hsk)
            pc <= pc_nxt;
    end

    logic [`RV_PC_SIZE-1:0] pc_of_ir;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_of_ir <= {`RV_PC_SIZE{1'b0}};
        else if (ex_req_pkt_valid_set)
            pc_of_ir <= pc;
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ex_rsp_slv.rdy <= 1'b0;
        else if (fch_req_hsk)
            ex_rsp_slv.rdy <= 1'b1;
        else if (fch_rsp_hsk)
            ex_rsp_slv.rdy <= 1'b0;
    end

    assign ex_req_mst.vld = ir_valid_flag;
    assign ex_req_mst.pkt.ir.raw = ir;
    assign ex_req_mst.pkt.pc = pc_of_ir;
    assign ex_req_mst.pkt.pred_taken = 1'b0;
    assign ex_req_mst.pkt.pred_pc = {`RV_PC_SIZE{1'b0}};

    assign fch_req_mst.vld = (~fch_req_pend_flag) | fch_req_pend_clear;
    assign fch_req_mst.pkt.pc = pc_nxt;
    assign fch_rsp_slv.rdy = (~ir_valid_flag) | ir_valid_clear;

    assign fl_req_mst.vld = taken;

endmodule