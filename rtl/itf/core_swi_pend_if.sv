interface core_swi_pend_if_t;

    struct packed {
        logic msip;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
