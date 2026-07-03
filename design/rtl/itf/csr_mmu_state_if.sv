interface csr_mmu_state_if_t;

    struct packed {
        logic [31:0] satp;
        logic [31:0] mstatus;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
