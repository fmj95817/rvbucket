`include "bti.svh"


interface bti_req_if_t #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32
);
    logic vld;
    logic rdy;

    struct packed {
        logic [`BTI_TIDW-1:0]  tid;
        bti_cmd_t              cmd;
        logic [BTI_AW-1:0]     addr;
        logic [BTI_DW-1:0]     data;
        logic [BTI_DW/8-1:0]   strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface

interface bti_rsp_if_t #(
    parameter BTI_DW = 32
);
    logic vld;
    logic rdy;
    struct packed {
        logic [`BTI_TIDW-1:0] tid;
        logic [BTI_DW-1:0]    data;
        logic                 ok;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface