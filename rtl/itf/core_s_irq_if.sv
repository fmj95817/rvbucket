interface core_s_irq_if_t;

    struct packed {
        logic ssw;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
