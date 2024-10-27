`include "bus.svh"

interface bus_trans_if #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32
);
    logic req_vld;
    logic req_rdy;
    struct packed {
        bus_cmd_t            cmd;
        logic [BTI_AW-1:0]   addr;
        logic [BTI_DW-1:0]   data;
        logic [BTI_DW/8-1:0] strobe;
    } req_pkt;

    logic rsp_vld;
    logic rsp_rdy;
    struct packed {
        logic [BTI_DW-1:0] data;
        logic              ok;
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