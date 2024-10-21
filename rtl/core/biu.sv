module biu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,
    ifetch_if.slave   ifetch,
    sram_if.master    sram_rw
);
    assign ifetch.req_rdy = 1'b1;

    tri ifu_biu_trans = ifetch.req_vld & ifetch.req_rdy;

    logic inst_valid;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            inst_valid <= 1'b0;
        else
            inst_valid <= ifu_biu_trans;
    end
    assign ifetch.rsp_vld = inst_valid;
    assign ifetch.rsp_ir = sram_rw.rdata;

    assign sram_rw.addr = ifetch.req_pc;
    assign sram_rw.wdata = {DW{32'b0}};
    assign sram_rw.wen = 1'b0;

endmodule