module biu #(
    parameter AW = 32,
    parameter DW = 32,
    parameter SRAM_AW = 15
)(
    input              clk,
    input              rst_n,
    ifetch_if_t.slave  ifetch,
    sram_if_t.master   sram_rw
);
    assign ifetch.req_rdy = 1'b1;

    tri ifetch_req_hsk = ifetch.req_vld & ifetch.req_rdy;

    logic inst_valid;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            inst_valid <= 1'b0;
        else
            inst_valid <= ifetch_req_hsk;
    end
    assign ifetch.rsp_vld = inst_valid;
    assign ifetch.rsp_ir = sram_rw.rdata;

    assign sram_rw.addr = ifetch.req_pc[SRAM_AW+1:2];
    assign sram_rw.wen = 1'b0;

endmodule