module bti_to_rom #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter ROM_AW = 15
)(
    input               clk,
    input               rst_n,
    bus_trans_if.slave  bti,
    output [ROM_AW-1:0] rom_addr,
    input [BTI_DW-1:0]  rom_data
);
    tri bti_req_hsk = bti.req_vld & bti.req_rdy;
    tri bti_rsp_hsk = bti.rsp_vld & bti.rsp_rdy;

    tri bti_req_pend_flag;
    tri bti_req_pend_set = bti_req_hsk;
    tri bti_req_pend_clear = bti_rsp_hsk;
    lib_flag u_bti_req_pend_flag(clk, rst_n,
        bti_req_pend_set, bti_req_pend_clear, bti_req_pend_flag);

    tri [ROM_AW-1:0] req_rom_addr = bti.req_pkt.addr[ROM_AW+1:2];
    logic [ROM_AW-1:0] rom_addr_pend;
    always_ff @(posedge clk) begin
        if (bti_req_hsk)
            rom_addr_pend <= #1 req_rom_addr;
    end

    assign bti.req_rdy = 1'b1;
    assign bti.rsp_vld = bti_req_pend_flag;
    assign bti.rsp_pkt.data = rom_data;

    assign rom_addr = bti_req_hsk ? req_rom_addr : rom_addr_pend;
endmodule

module bti_rom #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter ROM_AW = 15
)(
    input              clk,
    input              rst_n,
    bus_trans_if.slave bti
);
    tri [ROM_AW-1:0] rom_addr;
    tri [BTI_DW-1:0] rom_data;

    rom #(ROM_AW, BTI_DW) u_rom(.*);
    bti_to_rom #(BTI_AW, BTI_DW, ROM_AW) u_bti_to_rom(.*);
endmodule