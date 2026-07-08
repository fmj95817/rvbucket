`include "itf/ldst_req_if.svh"

interface ldst_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] addr;
        logic st;
        ldst_req_cmo_t cmo;
        ldst_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
