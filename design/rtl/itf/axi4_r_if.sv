`include "itf/axi4_r_if.svh"
`include "dbg/itf_trace.svh"
`include "dbg/itf_checker.svh"

interface axi4_r_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [7:0] id;
        logic [31:0] data;
        axi4_r_resp_t resp;
        logic last;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%02x %08x %01x %01x",
            pkt.id,
            pkt.data,
            pkt.resp,
            pkt.last
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_VLD_RDY_CHECK(pkt)

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
