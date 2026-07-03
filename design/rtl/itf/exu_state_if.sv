interface exu_state_if_t;

    struct packed {
        logic [1:0] priv;
        logic [31:0] pc;
        logic [31:0] irq_epc;
        logic irq_defer;
        logic wfi;
        logic [31:0] wfi_resume_pc;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
