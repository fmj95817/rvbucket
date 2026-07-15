`include "dbg/itf_trace.svh"

interface perf_ifu_if_t;

    struct packed {
        logic fch_rsp_inst;
        logic fch_ost_full;
        logic fch_rspq_bp;
        logic ctrlq_bp;
        logic ex_req_bp;
        logic fch_req_bp;
        logic pred_wait;
        logic issue_hold;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.fch_rsp_inst,
            pkt.fch_ost_full,
            pkt.fch_rspq_bp,
            pkt.ctrlq_bp,
            pkt.ex_req_bp,
            pkt.fch_req_bp,
            pkt.pred_wait,
            pkt.issue_hold
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
