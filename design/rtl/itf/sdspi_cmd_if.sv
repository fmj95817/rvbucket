`include "itf/sdspi_cmd_if.svh"
`include "dbg/itf_trace.svh"

interface sdspi_cmd_if_t;
    logic vld;
    logic rdy;

    struct packed {
        sdspi_cmd_kind_t kind;
        logic enable;
        logic [31:0] clock_div;
        logic [5:0] opcode;
        logic [31:0] arg;
        logic [2:0] rsp_type;
        logic data_present;
        logic write;
        logic [31:0] block_size;
        logic [31:0] block_count;
        logic [31:0] data_len;
        logic [31:0] data;
        logic last;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%01x %01x %08x %02x %08x %01x %01x %01x %08x %08x %08x %08x %01x",
            pkt.kind,
            pkt.enable,
            pkt.clock_div,
            pkt.opcode,
            pkt.arg,
            pkt.rsp_type,
            pkt.data_present,
            pkt.write,
            pkt.block_size,
            pkt.block_count,
            pkt.data_len,
            pkt.data,
            pkt.last
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
