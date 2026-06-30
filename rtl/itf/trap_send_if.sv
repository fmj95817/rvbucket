interface trap_send_if_t;
    logic vld;

    struct packed {
        logic [31:0] target_pc;
    } pkt;

    modport mst (output vld, pkt);
    modport slv (input vld, pkt);
endinterface
