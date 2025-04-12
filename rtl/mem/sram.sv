`include "bti.svh"

interface sram_rw_if_t #(
    parameter AW = 15,
    parameter DW = 32
);
    logic           cs;
    logic  [AW-1:0] addr;
    logic           wen;
    logic  [DW-1:0] wdata;
    logic  [DW-1:0] rdata;

    modport mst (output cs, addr, wen, wdata, input rdata);
    modport slv (input cs, addr, wen, wdata, output rdata);
endinterface

module bti_to_sram #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter SRAM_AW = 15
)(
    input logic        clk,
    input logic        rst_n,
    bti_req_if_t.slv   bti_req_slv,
    bti_rsp_if_t.mst   bti_rsp_mst,
    sram_rw_if_t.mst   sram_rw_mst
);
    reg_slice #(
        .DW(`BTI_TIDW)
    ) u_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .src_vld  (bti_req_slv.vld),
        .src_rdy  (bti_req_slv.rdy),
        .src_data (bti_req_slv.pkt.tid),
        .dst_vld  (bti_rsp_mst.vld),
        .dst_rdy  (bti_rsp_mst.rdy),
        .dst_data (bti_rsp_mst.pkt.tid)
    );

    assign sram_rw_mst.cs = bti_req_slv.vld & bti_req_slv.rdy;
    assign sram_rw_mst.addr = bti_req_slv.pkt.addr[SRAM_AW+1:2];
    assign sram_rw_mst.wen = bti_req_slv.pkt.cmd == BTI_CMD_WRITE;
    assign sram_rw_mst.wdata = bti_req_slv.pkt.data;

    assign bti_rsp_mst.pkt.data = sram_rw_mst.rdata;
    assign bti_rsp_mst.pkt.ok = 1'b1;
endmodule

module bti_sram #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter SRAM_AW = 15
)(
    input logic        clk,
    input logic        rst_n,
    bti_req_if_t.slv   bti_req_slv,
    bti_rsp_if_t.mst   bti_rsp_mst
);
    sram_rw_if_t #(SRAM_AW, BTI_DW) sram_rw_if();

    sram #(
        .AW          (SRAM_AW),
        .DW          (BTI_DW)
    ) u_sram(
        .clk         (clk),
        .sram_rw_slv (sram_rw_if)
    );

    bti_to_sram #(
        .BTI_AW      (BTI_AW),
        .BTI_DW      (BTI_DW),
        .SRAM_AW     (SRAM_AW)
    ) u_bti_to_sram(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (bti_req_slv),
        .bti_rsp_mst (bti_rsp_mst),
        .sram_rw_mst (sram_rw_if)
    );
endmodule