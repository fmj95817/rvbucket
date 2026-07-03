interface csr_trap_state_if_t;

    struct packed {
        logic [31:0] mstatus;
        logic [31:0] mip;
        logic [31:0] mie;
        logic [31:0] mtvec;
        logic [31:0] mepc;
        logic [31:0] medeleg;
        logic [31:0] mideleg;
        logic [31:0] sstatus;
        logic [31:0] sip;
        logic [31:0] sie;
        logic [31:0] stvec;
        logic [31:0] sepc;
    } pkt;

    modport mst (output pkt);
    modport slv (input pkt);
endinterface
