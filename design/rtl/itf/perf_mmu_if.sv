`include "dbg/itf_trace.svh"

interface perf_mmu_if_t;

    struct packed {
        logic itlb_hit;
        logic itlb_miss;
        logic dtlb_hit;
        logic dtlb_miss;
        logic i_stg_full;
        logic d_stg_full;
        logic i_ost_full;
        logic d_ost_full;
        logic ptw_busy;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.itlb_hit,
            pkt.itlb_miss,
            pkt.dtlb_hit,
            pkt.dtlb_miss,
            pkt.i_stg_full,
            pkt.d_stg_full,
            pkt.i_ost_full,
            pkt.d_ost_full,
            pkt.ptw_busy
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
