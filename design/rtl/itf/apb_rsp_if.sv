`include "dbg/itf_trace.svh"

interface apb_rsp_if_t;
    logic pready;

    struct packed {
        logic [31:0] prdata;
        logic pslverr;
    } pkt;

`ifdef RVB_ITF_TRACE_ENABLED
    logic [$bits(pkt)-1:0] __itf_trace_old_pkt;
    bit __itf_trace_pkt_seen;
    localparam int __ITF_TRACE_CTRL_WIDTH = 1;
    logic [__ITF_TRACE_CTRL_WIDTH-1:0] __itf_trace_old_ctrl;
    bit __itf_trace_ctrl_seen;

    initial begin
        __itf_trace_old_pkt = '0;
        __itf_trace_pkt_seen = 1'b0;
        __itf_trace_old_ctrl = '0;
        __itf_trace_ctrl_seen = 1'b0;
    end

    function automatic bit __itf_trace_pkt_changed;
        if (!__itf_trace_pkt_seen)
            __itf_trace_pkt_changed = 1'b1;
        else
            __itf_trace_pkt_changed = __itf_trace_old_pkt !== $bits(pkt)'(pkt);
    endfunction

    function automatic logic [__ITF_TRACE_CTRL_WIDTH-1:0] __itf_trace_ctrl_pack;
        __itf_trace_ctrl_pack = {pready};
    endfunction

    function automatic bit __itf_trace_ctrl_changed;
        if (!__itf_trace_ctrl_seen)
            __itf_trace_ctrl_changed = 1'b0;
        else
            __itf_trace_ctrl_changed = __itf_trace_old_ctrl !==
                __itf_trace_ctrl_pack();
    endfunction

    task automatic __itf_trace_update_state;
        begin
            __itf_trace_old_pkt = $bits(pkt)'(pkt);
            __itf_trace_pkt_seen = 1'b1;
            __itf_trace_old_ctrl = __itf_trace_ctrl_pack();
            __itf_trace_ctrl_seen = 1'b1;
        end
    endtask

    function automatic string __itf_trace_pkt_str;
        __itf_trace_pkt_str = $sformatf(
            "%08x %01x",
            pkt.prdata,
            pkt.pslverr
        );
    endfunction
`endif
    modport mst (output pkt, pready);
    modport slv (input pkt, pready);

    `RVB_ITF_TRACE_WHEN_UPDATE("mst", "slv", __itf_trace_ctrl_changed())
endinterface
