interface ldst_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] addr;
        logic st;
        logic [31:0] data;
        logic [3:0] strobe;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
