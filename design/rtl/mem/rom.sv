`include "itf/bti_req_if.svh"

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
    output logic [ROM_AW-3:0] addr,
    input logic [BTI_DW-1:0]  data
);
    logic [1:0] rsp_addr_low;

    reg_slice #(
        .DW       (16)
    ) u_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .src_vld  (bti_req_slv.vld),
        .src_rdy  (bti_req_slv.rdy),
        .src_data (bti_req_slv.pkt.trans_id),
        .dst_vld  (bti_rsp_mst.vld),
        .dst_rdy  (bti_rsp_mst.rdy),
        .dst_data (bti_rsp_mst.pkt.trans_id)
    );

    assign cs = bti_req_slv.vld & bti_req_slv.rdy;
    assign addr = bti_req_slv.pkt.addr[ROM_AW-1:2];

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            rsp_addr_low <= 2'b0;
        else if (cs)
            rsp_addr_low <= bti_req_slv.pkt.addr[1:0];
    end

    always_comb begin
        case (rsp_addr_low)
            2'b00: bti_rsp_mst.pkt.data = data;
            2'b01: bti_rsp_mst.pkt.data = {{8{1'b0}}, data[BTI_DW-1:8]};
            2'b10: bti_rsp_mst.pkt.data = {{16{1'b0}}, data[BTI_DW-1:16]};
            2'b11: bti_rsp_mst.pkt.data = {{24{1'b0}}, data[BTI_DW-1:24]};
            default: bti_rsp_mst.pkt.data = {BTI_DW{1'bx}};
        endcase
    end

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
    tri [ROM_AW-3:0] addr;
    tri [BTI_DW-1:0] data;

    rom #(
        .AW          (ROM_AW - 2),
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
