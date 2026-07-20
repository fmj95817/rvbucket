`include "dbg/itf_trace.svh"
`include "dbg/itf_checker.svh"

interface fch_rsp_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] ir;
        logic ok;
        logic expt;
        logic [31:0] cause;
        logic [1:0] priv;
        logic [31:0] tval;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%08x %01x %01x %08x %01x %08x",
            pkt.ir,
            pkt.ok,
            pkt.expt,
            pkt.cause,
            pkt.priv,
            pkt.tval
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_VLD_RDY_CHECK(pkt)

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
