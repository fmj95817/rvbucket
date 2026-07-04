`include "spec/core/isa.svh"

module ifu(
    input logic      clk,
    input logic      rst_n,
    fch_req_if_t.mst fch_req_mst,
    fch_rsp_if_t.slv fch_rsp_slv,
    ex_req_if_t.mst  ex_req_mst,
    ex_rsp_if_t.slv  ex_rsp_slv,
    fl_req_if_t.mst  fl_req_mst
);
    typedef enum logic [1:0] {
        FETCH_REQ,
        FETCH_RESP,
        EXEC
    } state_t;

    state_t state;
    logic [`RV_PC_SIZE-1:0] pc;
    logic [`RV_PC_SIZE-1:0] req_pc;
    logic [`RV_IR_SIZE-1:0] ir_raw;

    wire fch_req_hsk = fch_req_mst.vld && fch_req_mst.rdy;
    wire fch_rsp_hsk = fch_rsp_slv.vld && fch_rsp_slv.rdy;
    wire ex_req_hsk = ex_req_mst.vld && ex_req_mst.rdy;
    wire redirect = ex_rsp_slv.vld && ex_rsp_slv.rdy && ex_rsp_slv.pkt.taken;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= FETCH_REQ;
            pc <= 32'h40000000;
            req_pc <= '0;
            ir_raw <= '0;
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
                        state <= EXEC;
                    end
                end
                EXEC: begin
                    if (ex_req_hsk) begin
                        pc <= redirect ? ex_rsp_slv.pkt.target_pc : req_pc + 32'd4;
                        state <= FETCH_REQ;
                    end
                end
                default: state <= FETCH_REQ;
            endcase
        end
    end

    assign fch_req_mst.vld = state == FETCH_REQ;
    assign fch_req_mst.pkt.pc = pc;
    assign fch_rsp_slv.rdy = state == FETCH_RESP;

    assign ex_req_mst.vld = state == EXEC;
    assign ex_req_mst.pkt.inst.raw = ir_raw;
    assign ex_req_mst.pkt.pc = req_pc;
    assign ex_req_mst.pkt.pred_taken = 1'b0;
    assign ex_req_mst.pkt.pred_pc = '0;
    assign ex_rsp_slv.rdy = state == EXEC;

    assign fl_req_mst.vld = 1'b0;
endmodule
