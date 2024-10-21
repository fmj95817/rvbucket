module ifu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input             clk,
    input             rst_n,
    ifetch_if.master  ifetch,
    iexec_if.master   iexec
);
    tri ifu_biu_trans = ifetch.req_vld & ifetch.req_rdy;
    tri biu_ifu_trans = ifetch.rsp_vld & ifetch.rsp_rdy;
    tri ifu_exu_trans = iexec.req_vld & iexec.req_rdy;

    tri ir_valid_set = biu_ifu_trans;
    tri ir_valid_clear = ifu_exu_trans;

    logic ir_valid;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir_valid <= 1'b0;
        else if (ir_valid_set | ir_valid_clear)
            ir_valid <= ir_valid_set | (~ir_valid_clear);
    end
    assign iexec.req_vld = ir_valid;
    assign ifetch.rsp_rdy = (~ir_valid) | ir_valid_clear;

    tri if_req_set = ifu_biu_trans;
    tri if_req_clear = biu_ifu_trans;

    logic if_req;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            if_req <= 1'b0;
        else if (if_req_set | if_req_clear)
            if_req <= if_req_set | (~if_req_clear);
    end
    assign ifetch.req_vld = (~if_req) | if_req_clear;

    logic [DW-1:0] ir;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir <= {DW{1'b0}};
        else if (ir_valid_set)
            ir <= ifetch.rsp_ir;
    end
    assign iexec.req_pkt.ir = ir;

    logic [2:0] pc_offset;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_offset <= {3{1'b0}};
        else
            pc_offset <= 3'd4;
    end

    logic [AW-1:0] pc;
    tri [AW-1:0] pc_nxt = pc + (iexec.rsp_pkt.taken ? iexec.rsp_pkt.offset : pc_offset);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc <= {DW{1'b0}} + 2'd2;
        else if (ifu_biu_trans)
            pc <= pc_nxt;
    end
    assign ifetch.req_pc = pc_nxt;
    assign iexec.req_pkt.pc = pc;

endmodule