interface ext_irq_if_t;

    struct packed {
        logic irq;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
