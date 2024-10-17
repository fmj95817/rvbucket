module rv32i #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,

    output  [AW-1:0]  addr,
    input   [DW-1:0]  rdata,
    output  [DW-1:0]  wdata,
    output            wen
);

    tri            ifu2biu_req_vld;
    tri            ifu2biu_req_rdy;
    tri  [AW-1:0]  ifu2biu_req_pc;

    tri            biu2ifu_rsp_vld;
    tri            biu2ifu_rsp_rdy;
    tri  [DW-1:0]  biu2ifu_rsp_inst;

    tri            ifu2exu_req_vld;
    tri            ifu2exu_req_rdy;
    tri  [DW-1:0]  ifu2exu_req_ir;
    tri  [AW-1:0]  ifu2exu_req_pc;

    tri            exu2ifu_taken;
    tri  [DW-1:0]  exu2ifu_offset;

    ifu u_ifu(.*);
    exu u_exu(.*);
    biu u_biu(.*);

endmodule