interface bti_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [15:0] trans_id;
        logic [31:0] data;
        logic ok;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
