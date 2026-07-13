`include "dbg/itf_trace.svh"

interface perf_bpu_if_t;

    struct packed {
        logic branch;
        logic pred_true;
        logic pred_false;
        logic cond_branch;
        logic cond_branch_pred_true;
        logic jal;
        logic jal_pred_true;
        logic jalr;
        logic jalr_pred_true;
        logic cond_bht_hit;
        logic jalr_ras_hit;
        logic jalr_btb_hit;
        logic jalr_btb_miss;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.branch,
            pkt.pred_true,
            pkt.pred_false,
            pkt.cond_branch,
            pkt.cond_branch_pred_true,
            pkt.jal,
            pkt.jal_pred_true,
            pkt.jalr,
            pkt.jalr_pred_true,
            pkt.cond_bht_hit,
            pkt.jalr_ras_hit,
            pkt.jalr_btb_hit,
            pkt.jalr_btb_miss
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
