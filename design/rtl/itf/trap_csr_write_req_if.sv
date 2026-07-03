interface trap_csr_write_req_if_t;
    logic vld;

    struct packed {
        logic [11:0] addr;
        logic [31:0] val;
    } pkt;

    modport mst (output vld, pkt);
    modport slv (input vld, pkt);
endinterface
