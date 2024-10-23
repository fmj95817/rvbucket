module exu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,
    iexec_if_t.slave  iexec,
    ldst_if_t.master  ldst_src
);
    assign iexec.req_rdy = 1'b1;

    tri ifu_exu_trans = iexec.req_vld & iexec.req_rdy;
    assign iexec.rsp_pkt.taken = ifu_exu_trans & (iexec.req_pkt.ir == 'h4f);
    assign iexec.rsp_pkt.offset = 'h10;

    assign ldst_src.req_vld = 1'b0;
endmodule