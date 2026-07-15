`include "itf/ldst_req_if.svh"
`include "dbg/itf_trace.svh"

interface ldst_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [31:0] addr;
        logic st;
        ldst_req_cmo_t cmo;
        ldst_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%08x %01x %01x %01x %08x %01x",
            pkt.addr,
            pkt.st,
            pkt.cmo,
            pkt.size,
            pkt.data,
            pkt.strobe
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
