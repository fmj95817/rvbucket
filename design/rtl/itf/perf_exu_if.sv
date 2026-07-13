`include "dbg/itf_trace.svh"

interface perf_exu_if_t;

    struct packed {
        logic ex_req;
        logic alu_inst;
        logic branch_inst;
        logic ldst_inst;
        logic misc_inst;
        logic sys_inst;
        logic wfi_stall;
        logic flush_drop;
        logic alu_busy;
        logic branch_busy;
        logic ldst_busy;
        logic sys_busy;
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
            "%01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x %01x",
            pkt.ex_req,
            pkt.alu_inst,
            pkt.branch_inst,
            pkt.ldst_inst,
            pkt.misc_inst,
            pkt.sys_inst,
            pkt.wfi_stall,
            pkt.flush_drop,
            pkt.alu_busy,
            pkt.branch_busy,
            pkt.ldst_busy,
            pkt.sys_busy
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
