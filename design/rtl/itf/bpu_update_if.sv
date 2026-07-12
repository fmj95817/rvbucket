`include "dbg/itf_trace.svh"

interface bpu_update_if_t;

    struct packed {
        logic vld;
        logic [31:0] pc;
        logic [31:0] ir;
        logic taken;
        logic [31:0] target_pc;
        logic pred_taken;
        logic [31:0] pred_pc;
        logic pred_true;
        logic is_boot_code;
        logic pred_cond_bht_hit;
        logic pred_jalr_ras_hit;
        logic pred_jalr_btb_hit;
        logic pred_jalr_btb_miss;
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
            "%01x %08x %08x %01x %08x %01x %08x %01x %01x %01x %01x %01x %01x",
            pkt.vld,
            pkt.pc,
            pkt.ir,
            pkt.taken,
            pkt.target_pc,
            pkt.pred_taken,
            pkt.pred_pc,
            pkt.pred_true,
            pkt.is_boot_code,
            pkt.pred_cond_bht_hit,
            pkt.pred_jalr_ras_hit,
            pkt.pred_jalr_btb_hit,
            pkt.pred_jalr_btb_miss
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
