module ifu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,

    output            ifu2biu_req_vld,
    input             ifu2biu_req_rdy,
    output  [AW-1:0]  ifu2biu_req_pc,

    input             biu2ifu_rsp_vld,
    output            biu2ifu_rsp_rdy,
    input   [DW-1:0]  biu2ifu_rsp_inst,

    output            ifu2exu_req_vld,
    input             ifu2exu_req_rdy,
    output  [DW-1:0]  ifu2exu_req_ir,
    output  [AW-1:0]  ifu2exu_req_pc,

    input             exu2ifu_taken,
    input   [DW-1:0]  exu2ifu_offset
);
    tri ifu_biu_trans = ifu2biu_req_vld & ifu2biu_req_rdy;
    tri biu_ifu_trans = biu2ifu_rsp_vld & biu2ifu_rsp_rdy;
    tri ifu_exu_trans = ifu2exu_req_vld & ifu2exu_req_rdy;

    tri ir_valid_set = biu_ifu_trans;
    tri ir_valid_clear = ifu_exu_trans;

    logic ir_valid;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir_valid <= 1'b0;
        else if (ir_valid_set | ir_valid_clear)
            ir_valid <= ir_valid_set | (~ir_valid_clear);
    end
    assign ifu2exu_req_vld = ir_valid;
    assign biu2ifu_rsp_rdy = (~ir_valid) | ir_valid_clear;

    tri if_req_set = ifu_biu_trans;
    tri if_req_clear = biu_ifu_trans;

    logic if_req;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            if_req <= 1'b0;
        else if (if_req_set | if_req_clear)
            if_req <= if_req_set | (~if_req_clear);
    end
    assign ifu2biu_req_vld = (~if_req) | if_req_clear;

    logic [DW-1:0] ir;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir <= {DW{1'b0}};
        else if (ir_valid_set)
            ir <= biu2ifu_rsp_inst;
    end
    assign ifu2exu_req_ir = ir;

    logic [2:0] pc_offset;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_offset <= {3{1'b0}};
        else
            pc_offset <= 3'd4;
    end

    logic [AW-1:0] pc;
    tri [AW-1:0] pc_nxt = pc + (exu2ifu_taken ? exu2ifu_offset : pc_offset);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc <= {DW{1'b0}} + 2'd2;
        else if (ifu_biu_trans)
            pc <= pc_nxt;
    end
    assign ifu2biu_req_pc = pc_nxt;
    assign ifu2exu_req_pc = pc;

endmodule