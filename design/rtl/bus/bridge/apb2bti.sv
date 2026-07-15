`include "itf/bti_req_if.svh"

module apb2bti(
    input logic      clk,
    input logic      rst_n,
    apb_req_if_t.slv apb_req_slv,
    apb_rsp_if_t.mst apb_rsp_mst,
    bti_req_if_t.mst bti_req_mst,
    bti_rsp_if_t.slv bti_rsp_slv
);
    typedef enum logic [1:0] {
        S_IDLE,
        S_REQ,
        S_RSP,
        S_DONE
    } state_t;

    state_t state;
    state_t state_nxt;
    logic req_write;
    logic [31:0] req_addr;
    logic [31:0] req_data;
    logic [3:0] req_strb;
    logic [31:0] rsp_data;
    logic rsp_err;
    logic capture_req;
    logic capture_rsp;

    wire apb_req = apb_req_slv.psel && apb_req_slv.penable;
    wire bti_req_hsk = bti_req_mst.vld && bti_req_mst.rdy;
    wire bti_rsp_hsk = bti_rsp_slv.vld && bti_rsp_slv.rdy;

    always_comb begin
        state_nxt = state;
        capture_req = 1'b0;
        capture_rsp = 1'b0;

        unique case (state)
        S_IDLE: begin
            if (apb_req) begin
                capture_req = 1'b1;
                state_nxt = S_REQ;
            end
        end
        S_REQ: begin
            if (bti_req_hsk)
                state_nxt = S_RSP;
        end
        S_RSP: begin
            if (bti_rsp_hsk) begin
                capture_rsp = 1'b1;
                state_nxt = S_DONE;
            end
        end
        S_DONE: begin
            state_nxt = S_IDLE;
        end
        default: begin
            state_nxt = S_IDLE;
        end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE;
            req_write <= 1'b0;
            req_addr <= '0;
            req_data <= '0;
            req_strb <= '0;
            rsp_data <= '0;
            rsp_err <= 1'b0;
        end else begin
            state <= state_nxt;

            if (capture_req) begin
                req_write <= apb_req_slv.pkt.pwrite;
                req_addr <= apb_req_slv.pkt.paddr;
                req_data <= apb_req_slv.pkt.pwdata;
                req_strb <= apb_req_slv.pkt.pstrb;
            end

            if (capture_rsp) begin
                rsp_data <= bti_rsp_slv.pkt.data;
                rsp_err <= !bti_rsp_slv.pkt.ok;
            end
        end
    end

    assign bti_req_mst.vld = state == S_REQ;
    assign bti_req_mst.pkt.trans_id = '0;
    assign bti_req_mst.pkt.cmd = req_write ? BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
    assign bti_req_mst.pkt.addr = req_addr;
    assign bti_req_mst.pkt.size = BTI_REQ_SIZE_B4;
    assign bti_req_mst.pkt.data = req_data;
    assign bti_req_mst.pkt.strobe = req_strb;
    assign bti_rsp_slv.rdy = state == S_RSP;

    assign apb_rsp_mst.pready = state == S_DONE;
    assign apb_rsp_mst.pkt.prdata = rsp_data;
    assign apb_rsp_mst.pkt.pslverr = rsp_err;
endmodule
