`include "itf/hart_expt_if.svh"

interface hart_expt_if_t;
    logic vld;

    struct packed {
        hart_expt_type_t expt_type;
        hart_expt_cause_t cause;
        logic [1:0] priv;
        logic [31:0] pc;
        logic [31:0] tval;
    } pkt;

    modport mst (output vld, pkt);
    modport slv (input vld, pkt);
endinterface
