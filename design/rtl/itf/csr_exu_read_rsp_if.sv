interface csr_exu_read_rsp_if_t;

    struct packed {
        logic [31:0] val;
        logic ok;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
