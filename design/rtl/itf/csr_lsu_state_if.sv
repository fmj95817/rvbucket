interface csr_lsu_state_if_t;

    struct packed {
        logic [31:0] satp;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
