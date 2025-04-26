`include "core/isa.svh"

interface fch_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_PC_SIZE-1:0] pc;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface