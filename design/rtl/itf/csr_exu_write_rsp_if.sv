`include "dbg/itf_trace.svh"

interface csr_exu_write_rsp_if_t;
    logic vld;

    struct packed {
        logic ok;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%01x",
            pkt.ok
        );
    endfunction
`endif
    modport mst (output vld, pkt);
    modport slv (input vld, pkt);

    `RVB_ITF_TRACE_WHEN("drv", "", vld)
endinterface
