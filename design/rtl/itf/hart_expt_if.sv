`include "itf/hart_expt_if.svh"
`include "dbg/itf_trace.svh"

interface hart_expt_if_t;
    logic vld;

    struct packed {
        hart_expt_type_t expt_type;
        hart_expt_cause_t cause;
        logic [1:0] priv;
        logic [31:0] pc;
        logic [31:0] tval;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%01x %02x %01x %08x %08x",
            pkt.expt_type,
            pkt.cause,
            pkt.priv,
            pkt.pc,
            pkt.tval
        );
    endfunction
`endif
    modport mst (output vld, pkt);
    modport slv (input vld, pkt);

    `RVB_ITF_TRACE_WHEN("drv", "", vld)
endinterface
