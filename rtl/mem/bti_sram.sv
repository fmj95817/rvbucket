module bti_to_sram_if #(
    parameter AW = 32,
    parameter DW = 32,
    parameter SRAM_AW = 15
)(
    input                  clk,
    input                  rst_n,
    bus_trans_if_t.slave   bti,
    sram_if_t.master       sram_rw
);
    assign bti.req_rdy = 1'b1;

    tri bti_req_hsk = bti.req_vld & bti.req_rdy;
    tri bti_rsp_hsk = bti.rsp_vld & bti.rsp_rdy;

    tri bti_req_pend_flag;
    tri bti_req_pend_set = bti_req_hsk;
    tri bti_req_pend_clear = bti_rsp_hsk;
    lib_flag u_bti_req_pend_flag(clk, rst_n, bti_req_pend_set, bti_req_pend_clear, bti_req_pend_flag);

    tri [SRAM_AW-1:0] sram_addr = bti.req_pkt.addr[SRAM_AW+1:2];
    logic [SRAM_AW-1:0] sram_addr_pend;
    always_ff @(posedge clk) begin
        if (bti_req_hsk)
            sram_addr_pend <= #1 sram_addr;
    end

    assign bti.rsp_vld = bti_req_pend_flag;
    assign bti.rsp_pkt.data = sram_rw.rdata;

    assign sram_rw.addr = bti_req_hsk ? sram_addr : sram_addr_pend;
    assign sram_rw.wen = 1'b0;
endmodule

module bti_sram #(
    parameter AW = 32,
    parameter DW = 32,
    parameter SRAM_AW = 15
)(
    input                  clk,
    input                  rst_n,
    bus_trans_if_t.slave   bti
);
    sram_if_t sram_rw();

    sram u_sram(.*);
    bti_to_sram_if u_bti_to_sram_if(.*);
endmodule