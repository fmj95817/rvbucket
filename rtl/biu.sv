module biu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,

    input             ifu2biu_req_vld,
    output            ifu2biu_req_rdy,
    input   [AW-1:0]  ifu2biu_req_pc,

    output            biu2ifu_rsp_vld,
    input             biu2ifu_rsp_rdy,
    output  [DW-1:0]  biu2ifu_rsp_inst,

    output  [AW-1:0]  addr,
    input   [DW-1:0]  rdata,
    output  [DW-1:0]  wdata,
    output            wen
);
    assign ifu2biu_req_rdy = 1'b1;

    tri ifu_biu_trans = ifu2biu_req_vld & ifu2biu_req_rdy;

    logic inst_valid;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            inst_valid <= 1'b0;
        else
            inst_valid <= ifu_biu_trans;
    end
    assign biu2ifu_rsp_vld = inst_valid;
    assign biu2ifu_rsp_inst = rdata;

    assign addr = ifu2biu_req_pc;
    assign wdata = {DW{32'b0}};
    assign wen = 1'b0;

endmodule