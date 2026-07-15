`include "dbg/itf_trace.svh"

interface csr_trap_state_if_t;

    struct packed {
        logic [31:0] mstatus;
        logic [31:0] mip;
        logic [31:0] mie;
        logic [31:0] mtvec;
        logic [31:0] mepc;
        logic [31:0] medeleg;
        logic [31:0] mideleg;
        logic [31:0] sstatus;
        logic [31:0] sip;
        logic [31:0] sie;
        logic [31:0] stvec;
        logic [31:0] sepc;
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
            "%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x",
            pkt.mstatus,
            pkt.mip,
            pkt.mie,
            pkt.mtvec,
            pkt.mepc,
            pkt.medeleg,
            pkt.mideleg,
            pkt.sstatus,
            pkt.sip,
            pkt.sie,
            pkt.stvec,
            pkt.sepc
        );
    endfunction
`endif
    modport mst (output pkt);
    modport slv (input pkt);

    `RVB_ITF_TRACE_WHEN_UPDATE("drv", "", __itf_trace_pkt_changed())
endinterface
