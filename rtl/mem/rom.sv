`include "bti.svh"

module bti_to_rom #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter ROM_AW = 15
)(
    input logic               clk,
    input logic               rst_n,
    bti_req_if_t.slv          bti_req_slv,
    bti_rsp_if_t.mst          bti_rsp_mst,
    output logic              cs,
    output logic [ROM_AW-1:0] addr,
    input logic [BTI_DW-1:0]  data
);
    reg_slice #(
        .DW       (`BTI_TIDW)
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

    assign cs = bti_req_slv.vld & bti_req_slv.rdy;
    assign addr = bti_req_slv.pkt.addr[ROM_AW+1:2];
    assign bti_rsp_mst.pkt.data = data;
    assign bti_rsp_mst.pkt.ok = 1'b1;
endmodule

module bti_rom #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter ROM_AW = 15
)(
    input logic        clk,
    input logic        rst_n,
    bti_req_if_t.slv   bti_req_slv,
    bti_rsp_if_t.mst   bti_rsp_mst
);
    tri              cs;
    tri [ROM_AW-1:0] addr;
    tri [BTI_DW-1:0] data;

    rom #(
        .AW          (ROM_AW),
        .DW          (BTI_DW)
    ) u_rom(
        .clk         (clk),
        .cs          (cs),
        .addr        (addr),
        .data        (data)
    );

    bti_to_rom #(
        .BTI_AW      (BTI_AW),
        .BTI_DW      (BTI_DW),
        .ROM_AW      (ROM_AW)
    ) u_bti_to_rom(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (bti_req_slv),
        .bti_rsp_mst (bti_rsp_mst),
        .cs          (cs),
        .addr        (addr),
        .data        (data)
    );
endmodule