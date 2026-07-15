`include "dbg/itf_trace.svh"

interface trap_exu_ctrl_if_t;
    logic vld;

    struct packed {
        logic [1:0] priv;
        logic [31:0] irq_epc;
        logic wfi;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%01x %08x %01x",
            pkt.priv,
            pkt.irq_epc,
            pkt.wfi
        );
    endfunction
`endif
    modport mst (output vld, pkt);
    modport slv (input vld, pkt);

    `RVB_ITF_TRACE_WHEN("drv", "", vld)
endinterface
