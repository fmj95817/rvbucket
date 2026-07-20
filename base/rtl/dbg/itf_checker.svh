`ifndef RVB_ITF_CHECKER_SVH
`define RVB_ITF_CHECKER_SVH

`ifndef RVB_NO_ITF_CHECKER
`ifndef VERILATOR
`ifndef SYNTHESIS
`define RVB_ITF_CHECKER_ENABLED
`endif
`endif
`endif

`ifdef RVB_ITF_CHECKER_ENABLED
`ifndef RVB_ITF_CHECKER_TOP
`define RVB_ITF_CHECKER_TOP sim_top
`endif

`define RVB_ITF_VLD_RDY_CHECK(_PAYLOAD) \
    `RVB_ITF_VLD_RDY_CHECK_CANCEL(_PAYLOAD, 1'b0)

`define RVB_ITF_VLD_RDY_CHECK_CANCEL(_PAYLOAD, _CANCEL) \
    bit __itf_checker_enable; \
    bit __itf_checker_verbose; \
    bit __itf_checker_prev_stalled; \
    logic [$bits(_PAYLOAD)-1:0] __itf_checker_prev_payload; \
    longint unsigned __itf_checker_cycle; \
    longint unsigned __itf_checker_vld_hold_count; \
    longint unsigned __itf_checker_payload_stable_count; \
    string __itf_checker_name; \
    initial begin \
        __itf_checker_enable = $test$plusargs("ITF_CHECKER") || \
            $test$plusargs("itf_checker"); \
        __itf_checker_verbose = $test$plusargs("ITF_CHECKER_VERBOSE") || \
            $test$plusargs("itf_checker_verbose"); \
        __itf_checker_prev_stalled = 1'b0; \
        __itf_checker_prev_payload = '0; \
        __itf_checker_cycle = 0; \
        __itf_checker_vld_hold_count = 0; \
        __itf_checker_payload_stable_count = 0; \
        __itf_checker_name = $sformatf("%m"); \
    end \
    always @(posedge $root.`RVB_ITF_CHECKER_TOP.clk) begin \
        if ($root.`RVB_ITF_CHECKER_TOP.rst_n !== 1'b1) begin \
            __itf_checker_prev_stalled <= 1'b0; \
            __itf_checker_prev_payload <= '0; \
            __itf_checker_cycle <= 0; \
            __itf_checker_vld_hold_count <= 0; \
            __itf_checker_payload_stable_count <= 0; \
        end else if (__itf_checker_enable) begin \
            if (__itf_checker_prev_stalled && !(_CANCEL)) begin \
                if (vld !== 1'b1) begin \
                    if (__itf_checker_verbose || \
                        __itf_checker_vld_hold_count == 0) \
                        $display("RVB_ITF_CHECKER cycle=%0d interface=%s rule=VLD_HOLD prev_payload=%0h vld=%b rdy=%b", \
                            __itf_checker_cycle, __itf_checker_name, \
                            __itf_checker_prev_payload, vld, rdy); \
                    __itf_checker_vld_hold_count <= \
                        __itf_checker_vld_hold_count + 1; \
                end else if (_PAYLOAD !== __itf_checker_prev_payload) begin \
                    if (__itf_checker_verbose || \
                        __itf_checker_payload_stable_count == 0) \
                        $display("RVB_ITF_CHECKER cycle=%0d interface=%s rule=PAYLOAD_STABLE prev_payload=%0h payload=%0h vld=%b rdy=%b", \
                            __itf_checker_cycle, __itf_checker_name, \
                            __itf_checker_prev_payload, _PAYLOAD, vld, rdy); \
                    __itf_checker_payload_stable_count <= \
                        __itf_checker_payload_stable_count + 1; \
                end \
            end \
            __itf_checker_prev_stalled <= \
                !(_CANCEL) && (vld === 1'b1) && (rdy === 1'b0); \
            __itf_checker_prev_payload <= _PAYLOAD; \
            __itf_checker_cycle <= __itf_checker_cycle + 1; \
        end \
    end \
    final begin \
        if (__itf_checker_enable && \
            (__itf_checker_vld_hold_count != 0 || \
            __itf_checker_payload_stable_count != 0)) \
            $display("RVB_ITF_CHECKER_SUMMARY interface=%s vld_hold=%0d payload_stable=%0d", \
                __itf_checker_name, __itf_checker_vld_hold_count, \
                __itf_checker_payload_stable_count); \
    end
`else
`define RVB_ITF_VLD_RDY_CHECK(_PAYLOAD)
`define RVB_ITF_VLD_RDY_CHECK_CANCEL(_PAYLOAD, _CANCEL)
`endif

`endif
