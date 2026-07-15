`include "dbg/itf_trace.svh"

interface trap_csr_write_req_if_t;
    logic vld;

    struct packed {
        logic [11:0] addr;
        logic [31:0] val;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%03x %08x",
            pkt.addr,
            pkt.val
        );
    endfunction
`endif
    modport mst (output vld, pkt);
    modport slv (input vld, pkt);

    `RVB_ITF_TRACE_WHEN("drv", "", vld)
endinterface
