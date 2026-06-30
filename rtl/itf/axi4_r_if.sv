`include "itf/axi4_r_if.svh"

interface axi4_r_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [7:0] id;
        logic [31:0] data;
        axi4_r_resp_t resp;
        logic last;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
