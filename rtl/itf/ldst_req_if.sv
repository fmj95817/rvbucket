`include "core/isa.svh"

interface ldst_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_AW-1:0]     addr;
        logic                  st;
        logic [`RV_XLEN-1:0]   data;
        logic [`RV_XLEN/8-1:0] strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface