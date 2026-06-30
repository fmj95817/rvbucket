interface apb_rsp_if_t;
    logic pready;

    struct packed {
        logic [31:0] prdata;
        logic pslverr;
    } pkt;

    modport mst (output pkt, pready);
    modport slv (input pkt, pready);
endinterface
