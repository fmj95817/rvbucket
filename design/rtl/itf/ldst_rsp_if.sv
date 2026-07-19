`include "dbg/itf_trace.svh"
`include "dbg/itf_checker.svh"

interface ldst_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] data;
        logic ok;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%08x %01x",
            pkt.data,
            pkt.ok
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_VLD_RDY_CHECK(pkt)

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
