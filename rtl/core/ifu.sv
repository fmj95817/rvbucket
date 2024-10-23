module ifu #(
    parameter AW = 32,
    parameter DW = 32
)(
    input               clk,
    input               rst_n,
    ifetch_if_t.master  ifetch,
    iexec_if_t.master   iexec
);
    tri ifetch_req_hsk = ifetch.req_vld & ifetch.req_rdy;
    tri ifetch_rsp_hsk = ifetch.rsp_vld & ifetch.rsp_rdy;
    tri iexec_req_hsk = iexec.req_vld & iexec.req_rdy;

    tri ir_valid_flag;
    tri ifetch_req_pend_flag;
    tri iexec_req_pkt_valid;
    tri pc_pend_flag;

    tri ir_valid_set = ifetch_rsp_hsk;
    tri ir_valid_clear = iexec_req_hsk;
    lib_flag u_ir_valid_flag(clk, rst_n,
        ir_valid_set, ir_valid_clear, ir_valid_flag
    );

    tri ifetch_req_pend_set = ifetch_req_hsk;
    tri ifetch_req_pend_clear = ifetch_rsp_hsk;
    lib_flag u_ifetch_req_pend_flag(clk, rst_n,
        ifetch_req_pend_set, ifetch_req_pend_clear,
        ifetch_req_pend_flag
    );

    tri iexec_req_pkt_valid_set = pc_pend_flag & ifetch.rsp_rdy;
    tri iexec_req_pkt_valid_clear = iexec_req_hsk;
    lib_flag u_iexec_req_pkt_valid_flag(clk, rst_n,
        iexec_req_pkt_valid_set, iexec_req_pkt_valid_clear,
        iexec_req_pkt_valid
    );

    tri pc_pend_set = ifetch_req_hsk;
    tri pc_pend_clear = iexec_req_pkt_valid_set;
    lib_flag u_pc_pend_flag(clk, rst_n,
        pc_pend_set, pc_pend_clear, pc_pend_flag
    );

    logic [DW-1:0] ir;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ir <= {DW{1'b0}};
        else if (ir_valid_set)
            ir <= #1 ifetch.rsp_ir;
    end

    logic [2:0] pc_offset;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_offset <= {3{1'b0}};
        else
            pc_offset <= #1 3'd4;
    end

    logic [AW-1:0] pc;
    tri [AW-1:0] pc_nxt = pc + (iexec.rsp_pkt.taken ? iexec.rsp_pkt.offset : { {(AW-3){1'b0}}, pc_offset });
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc <= {AW{1'b0}};
        else if (ifetch_req_hsk)
            pc <= #1 pc_nxt;
    end

    logic [AW-1:0] pc_of_ir;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            pc_of_ir <= {AW{1'b0}};
        else if (iexec_req_pkt_valid_set)
            pc_of_ir <= #1 pc;
    end

    assign iexec.req_vld = ir_valid_flag;
    assign iexec.req_pkt.valid = iexec_req_pkt_valid;
    assign iexec.req_pkt.ir = ir;
    assign iexec.req_pkt.pc = pc_of_ir;

    assign ifetch.req_vld = (~ifetch_req_pend_flag) | ifetch_req_pend_clear;
    assign ifetch.req_pc = pc_nxt;
    assign ifetch.rsp_rdy = (~ir_valid_flag) | ir_valid_clear;
endmodule