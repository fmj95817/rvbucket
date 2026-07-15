`include "dbg/itf_trace.svh"

interface perf_rv32g_if_t;

    struct packed {
        logic hart_i_aw_bp;
        logic hart_i_w_bp;
        logic hart_i_ar_bp;
        logic hart_i_b_bp;
        logic hart_i_r_bp;
        logic hart_d_aw_bp;
        logic hart_d_w_bp;
        logic hart_d_ar_bp;
        logic hart_d_b_bp;
        logic hart_d_r_bp;
        logic cbi_i_aw_bp;
        logic cbi_i_w_bp;
        logic cbi_i_ar_bp;
        logic cbi_i_b_bp;
        logic cbi_i_r_bp;
        logic cbi_d_aw_bp;
        logic cbi_d_w_bp;
        logic cbi_d_ar_bp;
        logic cbi_d_b_bp;
        logic cbi_d_r_bp;
        logic l2_i_aw_bp;
        logic l2_i_w_bp;
        logic l2_i_ar_bp;
        logic l2_i_b_bp;
        logic l2_i_r_bp;
        logic l2_d_aw_bp;
        logic l2_d_w_bp;
        logic l2_d_ar_bp;
        logic l2_d_b_bp;
        logic l2_d_r_bp;
        logic cfg_apb_bp;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.hart_i_aw_bp,
            pkt.hart_i_w_bp,
            pkt.hart_i_ar_bp,
            pkt.hart_i_b_bp,
            pkt.hart_i_r_bp,
            pkt.hart_d_aw_bp,
            pkt.hart_d_w_bp,
            pkt.hart_d_ar_bp,
            pkt.hart_d_b_bp,
            pkt.hart_d_r_bp,
            pkt.cbi_i_aw_bp,
            pkt.cbi_i_w_bp,
            pkt.cbi_i_ar_bp,
            pkt.cbi_i_b_bp,
            pkt.cbi_i_r_bp,
            pkt.cbi_d_aw_bp,
            pkt.cbi_d_w_bp,
            pkt.cbi_d_ar_bp,
            pkt.cbi_d_b_bp,
            pkt.cbi_d_r_bp,
            pkt.l2_i_aw_bp,
            pkt.l2_i_w_bp,
            pkt.l2_i_ar_bp,
            pkt.l2_i_b_bp,
            pkt.l2_i_r_bp,
            pkt.l2_d_aw_bp,
            pkt.l2_d_w_bp,
            pkt.l2_d_ar_bp,
            pkt.l2_d_b_bp,
            pkt.l2_d_r_bp,
            pkt.cfg_apb_bp
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
