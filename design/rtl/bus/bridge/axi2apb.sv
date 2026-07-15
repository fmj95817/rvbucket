`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi2apb(
    input logic      clk,
    input logic      rst_n,
    axi4_aw_if_t.slv axi4_aw_slv,
    axi4_w_if_t.slv  axi4_w_slv,
    axi4_b_if_t.mst  axi4_b_mst,
    axi4_ar_if_t.slv axi4_ar_slv,
    axi4_r_if_t.mst  axi4_r_mst,
    apb_req_if_t.mst apb_req_mst,
    apb_rsp_if_t.slv apb_rsp_slv
);
    typedef enum logic [2:0] {
        S_IDLE,
        S_WR_WAIT,
        S_SETUP,
        S_ACCESS,
        S_RESP
    } state_t;

    state_t state;
    state_t state_nxt;
    logic aw_pending;
    logic w_pending;
    logic req_write;
    logic [7:0] req_id;
    logic [31:0] req_addr;
    logic [31:0] req_data;
    logic [3:0] req_strb;
    logic [31:0] rsp_data;
    logic rsp_err;
    logic capture_rsp;

    wire aw_hsk = axi4_aw_slv.vld && axi4_aw_slv.rdy;
    wire w_hsk = axi4_w_slv.vld && axi4_w_slv.rdy;
    wire ar_hsk = axi4_ar_slv.vld && axi4_ar_slv.rdy;
    wire write_complete = (aw_pending || aw_hsk) && (w_pending || w_hsk);
    wire apb_hsk = state == S_ACCESS && apb_rsp_slv.pready;
    wire axi_rsp_hsk = req_write ?
        (axi4_b_mst.vld && axi4_b_mst.rdy) :
        (axi4_r_mst.vld && axi4_r_mst.rdy);

    always_comb begin
        state_nxt = state;
        capture_rsp = 1'b0;

        unique case (state)
        S_IDLE: begin
            if (ar_hsk) begin
                state_nxt = S_SETUP;
            end else if (aw_hsk || w_hsk) begin
                state_nxt = write_complete ? S_SETUP : S_WR_WAIT;
            end
        end
        S_WR_WAIT: begin
            if (write_complete)
                state_nxt = S_SETUP;
        end
        S_SETUP: begin
            state_nxt = S_ACCESS;
        end
        S_ACCESS: begin
            if (apb_hsk) begin
                capture_rsp = 1'b1;
                state_nxt = S_RESP;
            end
        end
        S_RESP: begin
            if (axi_rsp_hsk)
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
            aw_pending <= 1'b0;
            w_pending <= 1'b0;
            req_write <= 1'b0;
            req_id <= '0;
            req_addr <= '0;
            req_data <= '0;
            req_strb <= '0;
            rsp_data <= '0;
            rsp_err <= 1'b0;
        end else begin
            state <= state_nxt;

            if (state == S_IDLE && ar_hsk) begin
                req_write <= 1'b0;
                req_id <= axi4_ar_slv.pkt.id;
                req_addr <= axi4_ar_slv.pkt.addr;
                req_data <= '0;
                req_strb <= '0;
            end

            if (aw_hsk) begin
                aw_pending <= 1'b1;
                req_id <= axi4_aw_slv.pkt.id;
                req_addr <= axi4_aw_slv.pkt.addr;
            end

            if (w_hsk) begin
                w_pending <= 1'b1;
                req_data <= axi4_w_slv.pkt.data;
                req_strb <= axi4_w_slv.pkt.strb;
            end

            if (state_nxt == S_SETUP && write_complete) begin
                req_write <= 1'b1;
                aw_pending <= 1'b0;
                w_pending <= 1'b0;
            end

            if (capture_rsp) begin
                rsp_data <= apb_rsp_slv.pkt.prdata;
                rsp_err <= apb_rsp_slv.pkt.pslverr;
            end
        end
    end

    assign axi4_aw_slv.rdy =
        (state == S_IDLE || state == S_WR_WAIT) &&
        !aw_pending &&
        !(state == S_IDLE && axi4_ar_slv.vld);
    assign axi4_w_slv.rdy =
        (state == S_IDLE || state == S_WR_WAIT) &&
        !w_pending &&
        !(state == S_IDLE && axi4_ar_slv.vld);
    assign axi4_ar_slv.rdy =
        state == S_IDLE &&
        !axi4_aw_slv.vld &&
        !axi4_w_slv.vld;

    assign apb_req_mst.psel = state == S_SETUP || state == S_ACCESS;
    assign apb_req_mst.penable = state == S_ACCESS;
    assign apb_req_mst.pkt.paddr = req_addr;
    assign apb_req_mst.pkt.pwrite = req_write;
    assign apb_req_mst.pkt.pwdata = req_data;
    assign apb_req_mst.pkt.pstrb = req_strb;

    assign axi4_r_mst.vld = state == S_RESP && !req_write;
    assign axi4_r_mst.pkt.id = req_id;
    assign axi4_r_mst.pkt.data = rsp_data;
    assign axi4_r_mst.pkt.resp = rsp_err ? AXI4_R_RESP_SLVERR : AXI4_R_RESP_OKAY;
    assign axi4_r_mst.pkt.last = 1'b1;

    assign axi4_b_mst.vld = state == S_RESP && req_write;
    assign axi4_b_mst.pkt.id = req_id;
    assign axi4_b_mst.pkt.resp = rsp_err ? AXI4_B_RESP_SLVERR : AXI4_B_RESP_OKAY;
endmodule
