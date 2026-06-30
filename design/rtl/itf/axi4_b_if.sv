`include "itf/axi4_b_if.svh"

interface axi4_b_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [7:0] id;
        axi4_b_resp_t resp;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
