interface csr_trap_write_rsp_if_t;
    logic vld;

    struct packed {
        logic ok;
    } pkt;

    modport mst (output vld, pkt);
    modport slv (input vld, pkt);
endinterface
