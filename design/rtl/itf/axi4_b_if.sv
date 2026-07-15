`include "itf/axi4_b_if.svh"
`include "dbg/itf_trace.svh"

interface axi4_b_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [7:0] id;
        axi4_b_resp_t resp;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%02x %01x",
            pkt.id,
            pkt.resp
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
