module exu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input            clk,
    input            rst_n,

    input            ifu2exu_req_vld,
    output   logic   ifu2exu_req_rdy,
    input  [DW-1:0]  ifu2exu_req_ir,
    input  [AW-1:0]  ifu2exu_req_pc,

    output           exu2ifu_taken,
    output [DW-1:0]  exu2ifu_offset
);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ifu2exu_req_rdy <= 1'b0;
        else
            ifu2exu_req_rdy <= 1'b1;
    end

    tri ifu_exu_trans = ifu2exu_req_vld & ifu2exu_req_rdy;
    assign exu2ifu_taken = ifu_exu_trans & (ifu2exu_req_ir == 'h4f);
    assign exu2ifu_offset = 'h10;

endmodule