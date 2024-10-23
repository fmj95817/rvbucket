module lsu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input               clk,
    input               rst_n,
    ldst_if_t.slave     ldst_src,
    ldst_if_t.master    ldst_gen
);
    assign ldst_gen.req_vld = ldst_src.req_vld;
    assign ldst_gen.req_pkt = ldst_src.req_pkt;
    assign ldst_src.req_rdy = ldst_gen.req_rdy;

    assign ldst_src.rsp_vld = ldst_gen.rsp_vld;
    assign ldst_src.rsp_pkt = ldst_gen.rsp_pkt;
    assign ldst_gen.rsp_rdy = ldst_src.rsp_rdy;
endmodule