`include "core/isa.svh"

interface fch_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_IR_SIZE-1:0] ir;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface