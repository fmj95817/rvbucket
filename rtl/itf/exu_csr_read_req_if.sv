interface exu_csr_read_req_if_t;

    struct packed {
        logic [11:0] addr;
        logic [1:0] priv;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
