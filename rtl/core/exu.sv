module exu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,
    iexec_if_t.slave  iexec
);

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            iexec.req_rdy <= 1'b0;
        else
            iexec.req_rdy <= 1'b1;
    end

    tri ifu_exu_trans = iexec.req_vld & iexec.req_rdy;
    assign iexec.rsp_pkt.taken = ifu_exu_trans & (iexec.req_pkt.ir == 'h4f);
    assign iexec.rsp_pkt.offset = 'h10;

endmodule