`include "itf/axi4_aw_if.svh"
`include "dbg/itf_trace.svh"

interface axi4_aw_if_t;
    logic vld;
    logic rdy;

    struct packed {
        logic [7:0] id;
        logic [31:0] addr;
        logic [7:0] len;
        axi4_aw_size_t size;
        axi4_aw_burst_t burst;
        logic lock;
        logic [3:0] cache;
        logic [2:0] prot;
        logic [3:0] qos;
        logic [31:0] user;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%02x %08x %02x %01x %01x %01x %01x %01x %01x %08x",
            pkt.id,
            pkt.addr,
            pkt.len,
            pkt.size,
            pkt.burst,
            pkt.lock,
            pkt.cache,
            pkt.prot,
            pkt.qos,
            pkt.user
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
