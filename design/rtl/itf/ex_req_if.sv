`include "spec/core/isa.svh"
`include "dbg/itf_trace.svh"

interface ex_req_if_t;
    logic vld;
    logic rdy;

    struct packed {
        rv32g_inst_t inst;
        logic [31:0] pc;
        logic pred_taken;
        logic [31:0] pred_pc;
        logic is_boot_code;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%08x %08x %01x %08x %01x",
            pkt.inst.raw,
            pkt.pc,
            pkt.pred_taken,
            pkt.pred_pc,
            pkt.is_boot_code
        );
    endfunction
`endif
    modport mst (output vld, pkt, input rdy);
    modport slv (input vld, pkt, output rdy);

    `RVB_ITF_TRACE_WHEN("mst", "slv", vld && rdy)
endinterface
