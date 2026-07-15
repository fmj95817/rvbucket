`include "dbg/itf_trace.svh"

interface trap_send_if_t;
    logic vld;

    struct packed {
        logic [31:0] target_pc;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%08x",
            pkt.target_pc
        );
    endfunction
`endif
    modport mst (output vld, pkt);
    modport slv (input vld, pkt);

    `RVB_ITF_TRACE_WHEN("drv", "", vld)
endinterface
