`include "itf/bti_req_if.svh"

interface bti_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        logic [31:0] data;
        logic [3:0] strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
