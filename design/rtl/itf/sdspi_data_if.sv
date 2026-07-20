`include "itf/sdspi_data_if.svh"
`include "dbg/itf_trace.svh"

interface sdspi_data_if_t;
    logic vld;
    logic rdy;

    struct packed {
        sdspi_data_kind_t kind;
        logic card_present;
        logic read_only;
        logic card_idle;
        logic [31:0] resp0;
        logic [31:0] resp1;
        logic [31:0] data;
        logic last;
        logic error;
        logic [31:0] cmd_error;
        logic [31:0] data_error;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%01x %01x %01x %01x %08x %08x %08x %01x %01x %08x %08x",
            pkt.kind,
            pkt.card_present,
            pkt.read_only,
            pkt.card_idle,
            pkt.resp0,
            pkt.resp1,
            pkt.data,
            pkt.last,
            pkt.error,
            pkt.cmd_error,
            pkt.data_error
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
