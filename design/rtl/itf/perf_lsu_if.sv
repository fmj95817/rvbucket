`include "dbg/itf_trace.svh"

interface perf_lsu_if_t;

    struct packed {
        logic exu_req;
        logic direct_req_bp;
        logic direct_rsp_wait;
        logic split_req;
        logic split_req_bp;
        logic split_rsp_wait;
        logic exu_rsp_bp;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED
    logic [$bits(pkt)-1:0] __itf_trace_old_pkt;
    bit __itf_trace_pkt_seen;

    initial begin
        __itf_trace_old_pkt = '0;
        __itf_trace_pkt_seen = 1'b0;
    end

    function automatic bit __itf_trace_pkt_changed;
        if (!__itf_trace_pkt_seen)
            __itf_trace_pkt_changed = 1'b1;
        else
            __itf_trace_pkt_changed = __itf_trace_old_pkt !== $bits(pkt)'(pkt);
    endfunction

    task automatic __itf_trace_update_state;
        begin
            __itf_trace_old_pkt = $bits(pkt)'(pkt);
            __itf_trace_pkt_seen = 1'b1;
        end
    endtask

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%01x %01x %01x %01x %01x %01x %01x",
            pkt.exu_req,
            pkt.direct_req_bp,
            pkt.direct_rsp_wait,
            pkt.split_req,
            pkt.split_req_bp,
            pkt.split_rsp_wait,
            pkt.exu_rsp_bp
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
