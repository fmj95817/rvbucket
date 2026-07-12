`include "dbg/itf_trace.svh"

interface l1_flush_if_t;
    logic vld;
    logic rdy;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = "00000000";
    endfunction
`endif
    modport mst (output vld, input rdy);
    modport slv (input vld, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
