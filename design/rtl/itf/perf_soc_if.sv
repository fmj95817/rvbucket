`include "dbg/itf_trace.svh"

interface perf_soc_if_t;

    struct packed {
        logic cycle;
        logic soc_mm_aw_bp;
        logic soc_mm_w_bp;
        logic soc_mm_ar_bp;
        logic soc_mm_b_bp;
        logic soc_mm_r_bp;
        logic soc_ddr_aw_bp;
        logic soc_ddr_w_bp;
        logic soc_ddr_ar_bp;
        logic soc_ddr_b_bp;
        logic soc_ddr_r_bp;
        logic soc_flash_aw_bp;
        logic soc_flash_w_bp;
        logic soc_flash_ar_bp;
        logic soc_flash_b_bp;
        logic soc_flash_r_bp;
        logic peri_apb_bp;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.cycle,
            pkt.soc_mm_aw_bp,
            pkt.soc_mm_w_bp,
            pkt.soc_mm_ar_bp,
            pkt.soc_mm_b_bp,
            pkt.soc_mm_r_bp,
            pkt.soc_ddr_aw_bp,
            pkt.soc_ddr_w_bp,
            pkt.soc_ddr_ar_bp,
            pkt.soc_ddr_b_bp,
            pkt.soc_ddr_r_bp,
            pkt.soc_flash_aw_bp,
            pkt.soc_flash_w_bp,
            pkt.soc_flash_ar_bp,
            pkt.soc_flash_b_bp,
            pkt.soc_flash_r_bp,
            pkt.peri_apb_bp
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
