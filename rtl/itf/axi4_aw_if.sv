`include "itf/axi4_aw_if.svh"

interface axi4_aw_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [7:0] id;
        logic [31:0] addr;
        logic [7:0] len;
        axi4_aw_size_t size;
        axi4_aw_burst_t burst;
        logic lock;
        logic [3:0] cache;
        logic [2:0] prot;
        logic [3:0] qos;
        logic [31:0] user;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
