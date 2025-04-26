`include "core/isa.svh"

interface ldst_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [`RV_XLEN-1:0] data;
        logic                ok;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
