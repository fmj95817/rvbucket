interface core_m_irq_if_t;

    struct packed {
        logic mtimer;
        logic msw;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
