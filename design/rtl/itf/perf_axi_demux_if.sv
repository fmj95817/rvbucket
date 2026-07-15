`include "dbg/itf_trace.svh"

interface perf_axi_demux_if_t;

    struct packed {
        logic rd_ost_full;
        logic wr_ost_full;
        logic ar_stg_full;
        logic aw_stg_full;
        logic w_stg_full;
        logic ar_issue_bp;
        logic aw_issue_bp;
        logic w_issue_bp;
        logic r_rsp_bp;
        logic b_rsp_bp;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.rd_ost_full,
            pkt.wr_ost_full,
            pkt.ar_stg_full,
            pkt.aw_stg_full,
            pkt.w_stg_full,
            pkt.ar_issue_bp,
            pkt.aw_issue_bp,
            pkt.w_issue_bp,
            pkt.r_rsp_bp,
            pkt.b_rsp_bp
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
