`include "bus.svh"

interface sram_if_t #(
    parameter AW = 15,
    parameter DW = 32
);
    logic  [AW-1:0] addr;
    logic           wen;
    logic  [DW-1:0] wdata;
    logic  [DW-1:0] rdata;

    modport master (output addr, wen, wdata, input rdata);
    modport slave (input addr, wen, wdata, output rdata);
endinterface

interface bus_trans_if_t #(
    parameter AW = 32,
    parameter DW = 32
);
    logic req_vld;
    logic req_rdy;
    struct packed {
        bus_cmd_t        cmd;
        logic [AW-1:0]   addr;
        logic [DW-1:0]   data;
        logic [DW/8-1:0] strobe;
    } req_pkt;

    logic rsp_vld;
    logic rsp_rdy;
    struct packed {
        logic [DW-1:0] data;
        logic          ok;
    } rsp_pkt;

    modport master (
        output req_vld, req_pkt, input req_rdy,
        input rsp_vld, rsp_pkt, output rsp_rdy
    );

    modport slave (
        input req_vld, req_pkt, output req_rdy,
        output rsp_vld, rsp_pkt, input rsp_rdy
    );
endinterface