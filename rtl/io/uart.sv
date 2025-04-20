`include "bti.svh"

module bti_to_uart #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter UART_AW = 2
)(
    input  logic        clk,
    input  logic        rst_n,
    bti_req_if_t.slv    bti_req_slv,
    bti_rsp_if_t.mst    bti_rsp_mst,
    output logic        ch_vld,
    output logic [7:0]  ch,
    input  logic        done
);
    tri bti_req_hsk = bti_req_slv.vld & bti_req_slv.rdy;
    tri bti_rsp_hsk = bti_rsp_mst.vld & bti_rsp_mst.rdy;

    logic req_pend;
    tri req_pend_set = bti_req_hsk;
    tri req_pend_clear = bti_rsp_hsk;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            req_pend <= 1'b0;
        else if (req_pend_set)
            req_pend <= 1'b1;
        else if (req_pend_clear)
            req_pend <= 1'b0;
    end

    logic done_pend;
    tri done_pend_set = done;
    tri done_pend_clear = bti_rsp_hsk;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            done_pend <= 1'b0;
        else if (done_pend_set)
            done_pend <= 1'b1;
        else if (done_pend_clear)
            done_pend <= 1'b0;
    end

    assign bti_req_slv.rdy = (~req_pend) | req_pend_clear;
    assign bti_rsp_mst.vld = done | done_pend;
    assign bti_rsp_mst.pkt.ok = 1'b1;
    assign bti_rsp_mst.pkt.data = {BTI_DW{1'b0}};

    assign ch_vld = bti_req_hsk;
    assign ch = bti_req_slv.pkt.data[7:0];

endmodule

module bti_uart #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter UART_AW = 2
)(
    input  logic        clk,
    input  logic        rst_n,
    bti_req_if_t.slv    bti_req_slv,
    bti_rsp_if_t.mst    bti_rsp_mst
);
    logic       ch_vld;
    logic [7:0] ch;
    logic       done;

    bti_to_uart #(
        .BTI_AW       (BTI_AW),
        .BTI_DW       (BTI_DW),
        .UART_AW      (UART_AW)
    ) u_bti_to_uart(
        .clk          (clk),
        .rst_n        (rst_n),
        .bti_req_slv  (bti_req_slv),
        .bti_rsp_mst  (bti_rsp_mst),
        .ch_vld       (ch_vld),
        .ch           (ch),
        .done         (done)
    );

    uart u_uart(
        .clk          (clk),
        .rst_n        (rst_n),
        .ch_vld       (ch_vld),
        .ch           (ch),
        .done         (done)
    );

endmodule