interface core_timer_if_t;

    struct packed {
        logic [63:0] mtime;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
