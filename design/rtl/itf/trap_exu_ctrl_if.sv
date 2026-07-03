interface trap_exu_ctrl_if_t;
    logic vld;

    struct packed {
        logic [1:0] priv;
        logic [31:0] irq_epc;
        logic wfi;
    } pkt;

    modport mst (output vld, pkt);
    modport slv (input vld, pkt);
endinterface
