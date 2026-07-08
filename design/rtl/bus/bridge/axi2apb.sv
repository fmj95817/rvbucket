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
    typedef enum logic [1:0] {
        S_IDLE,
        S_SETUP,
        S_ACCESS,
        S_RESP
    } state_t;

    state_t state;
    state_t state_nxt;
    logic req_write;
    logic [7:0] req_id;
    logic [31:0] req_addr;
    logic [31:0] req_data;
    logic [3:0] req_strb;
    logic [31:0] rsp_data;
    logic rsp_err;
    logic capture_req;
    logic capture_rsp;

    wire write_req = axi4_aw_slv.vld && axi4_w_slv.vld;
    wire read_req = axi4_ar_slv.vld && !write_req;
    wire apb_hsk = state == S_ACCESS && apb_rsp_slv.pready;
    wire axi_rsp_hsk = req_write ?
        (axi4_b_mst.vld && axi4_b_mst.rdy) :
        (axi4_r_mst.vld && axi4_r_mst.rdy);

    always_comb begin
        state_nxt = state;
        capture_req = 1'b0;
        capture_rsp = 1'b0;

        unique case (state)
        S_IDLE: begin
            if (write_req || read_req) begin
                capture_req = 1'b1;
                state_nxt = S_SETUP;
            end
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
            req_write <= 1'b0;
            req_id <= '0;
            req_addr <= '0;
            req_data <= '0;
            req_strb <= '0;
            rsp_data <= '0;
            rsp_err <= 1'b0;
        end else begin
            state <= state_nxt;

            if (capture_req) begin
                req_write <= write_req;
                req_id <= write_req ? axi4_aw_slv.pkt.id : axi4_ar_slv.pkt.id;
                req_addr <= write_req ? axi4_aw_slv.pkt.addr : axi4_ar_slv.pkt.addr;
                req_data <= write_req ? axi4_w_slv.pkt.data : '0;
                req_strb <= write_req ? axi4_w_slv.pkt.strb : '0;
            end

            if (capture_rsp) begin
                rsp_data <= apb_rsp_slv.pkt.prdata;
                rsp_err <= apb_rsp_slv.pkt.pslverr;
            end
        end
    end

    assign axi4_aw_slv.rdy = state == S_IDLE && write_req;
    assign axi4_w_slv.rdy = state == S_IDLE && write_req;
    assign axi4_ar_slv.rdy = state == S_IDLE && read_req;

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
