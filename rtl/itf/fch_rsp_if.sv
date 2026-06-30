interface fch_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] ir;
        logic ok;
        logic expt;
        logic [31:0] cause;
        logic [1:0] priv;
        logic [31:0] tval;
    } pkt;

    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);
endinterface
