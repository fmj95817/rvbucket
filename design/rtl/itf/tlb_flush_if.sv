`include "dbg/itf_trace.svh"

interface tlb_flush_if_t;
    logic vld;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = "00000000";
    endfunction
`endif
    modport mst (output vld);
    modport slv (input vld);

    `RVB_ITF_TRACE_WHEN_NOPKT("drv", "", vld)
endinterface
