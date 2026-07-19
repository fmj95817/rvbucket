`ifndef RVB_ITF_TRACE_SVH
`define RVB_ITF_TRACE_SVH

`ifndef RVB_NO_ITF_TRACE
`ifndef VERILATOR
`ifndef SYNTHESIS
`define RVB_ITF_TRACE_ENABLED
`endif
`endif
`endif

`ifdef RVB_ITF_TRACE_ENABLED
`define RVB_ITF_TRACE_FILTER_WIDTH 512
`define RVB_ITF_TRACE_DECL(_SUFFIX0, _SUFFIX1, _PAYLOAD_WIDTH) \
    import "DPI-C" function int rvb_itf_trace_filter_open( \
        input string name, input int payload_width); \
    import "DPI-C" function bit rvb_itf_trace_filter_eval( \
        input int filter_id, \
        input bit [`RVB_ITF_TRACE_FILTER_WIDTH-1:0] payload); \
    string __itf_trace_name; \
    integer __itf_trace_fd0; \
    integer __itf_trace_fd1; \
    bit __itf_trace_enable; \
    bit __itf_trace_inited; \
    integer __itf_trace_filter; \
    longint unsigned __itf_trace_cycle; \
    function automatic string __itf_trace_scope(input string raw); \
        string suffix; \
        int last; \
        begin \
            suffix = ".__itf_trace_init"; \
            __itf_trace_scope = raw; \
            if (raw.len() >= suffix.len()) begin \
                last = raw.len() - suffix.len(); \
                if (raw.substr(last, raw.len() - 1) == suffix) \
                    __itf_trace_scope = (last == 0) ? "" : raw.substr(0, last - 1); \
            end \
        end \
    endfunction \
    function automatic string __itf_trace_trim(input string s); \
        int last; \
        begin \
            while (s.len() != 0) begin \
                last = s.len() - 1; \
                if (s.getc(last) == 10 || s.getc(last) == 13 || \
                    s.getc(last) == 32 || s.getc(last) == 9) \
                    s = (last == 0) ? "" : s.substr(0, last - 1); \
                else \
                    break; \
            end \
            __itf_trace_trim = s; \
        end \
    endfunction \
    function automatic bit __itf_trace_list_match(input string target); \
        integer fd; \
        string line; \
        string item; \
        begin \
            __itf_trace_list_match = 1'b0; \
            if ($system("test -f itf_trace_list.txt") == 0) begin \
                fd = $fopen("itf_trace_list.txt", "r"); \
                while (!$feof(fd)) begin \
                    line = ""; \
                    void'($fgets(line, fd)); \
                    item = __itf_trace_trim(line); \
                    if (item.len() != 0 && item == target) \
                        __itf_trace_list_match = 1'b1; \
                end \
                $fclose(fd); \
            end \
        end \
    endfunction \
    task automatic __itf_trace_init; \
        begin \
            if (!__itf_trace_inited) begin \
                __itf_trace_inited = 1'b1; \
                __itf_trace_name = __itf_trace_scope($sformatf("%m")); \
                __itf_trace_enable = $test$plusargs("ITF_TRACE") || \
                    $test$plusargs("itf_trace") || \
                    __itf_trace_list_match(__itf_trace_name); \
                if (__itf_trace_enable) begin \
                    __itf_trace_filter = \
                        rvb_itf_trace_filter_open(__itf_trace_name, \
                            _PAYLOAD_WIDTH); \
                    void'($system("mkdir -p itf_trace")); \
                    __itf_trace_fd0 = $fopen($sformatf("itf_trace/%s_%s.txt", \
                        __itf_trace_name, _SUFFIX0), "w"); \
                    if (_SUFFIX1 != "") \
                        __itf_trace_fd1 = $fopen($sformatf("itf_trace/%s_%s.txt", \
                            __itf_trace_name, _SUFFIX1), "w"); \
                    else \
                        __itf_trace_fd1 = 0; \
                end \
            end \
        end \
    endtask \
    initial begin \
        __itf_trace_fd0 = 0; \
        __itf_trace_fd1 = 0; \
        __itf_trace_enable = 1'b0; \
        __itf_trace_inited = 1'b0; \
        __itf_trace_filter = 0; \
        __itf_trace_cycle = 0; \
        __itf_trace_init(); \
    end

`define RVB_ITF_TRACE_TICK \
    always @(posedge $root.sim_top.clk) begin \
        __itf_trace_cycle <= __itf_trace_cycle + 1; \
    end

`define RVB_ITF_TRACE_WRITE \
    $fdisplay(__itf_trace_fd0, "%016x %s", __itf_trace_cycle, \
        __itf_trace_pkt_str()); \
    if (__itf_trace_fd1 != 0) \
        $fdisplay(__itf_trace_fd1, "%016x %s", __itf_trace_cycle, \
            __itf_trace_pkt_str())

`define RVB_ITF_TRACE_WHEN_IMPL(_SUFFIX0, _SUFFIX1, _WHEN, _PAYLOAD_WIDTH, _PAYLOAD) \
    `RVB_ITF_TRACE_DECL(_SUFFIX0, _SUFFIX1, _PAYLOAD_WIDTH) \
    `RVB_ITF_TRACE_TICK \
    always @(posedge $root.sim_top.clk) begin \
        if (__itf_trace_enable && (_WHEN) && \
            (__itf_trace_filter == 0 || \
            rvb_itf_trace_filter_eval(__itf_trace_filter, \
                {{(`RVB_ITF_TRACE_FILTER_WIDTH-(_PAYLOAD_WIDTH)){1'b0}}, \
                _PAYLOAD}))) begin \
            `RVB_ITF_TRACE_WRITE; \
        end \
    end

`define RVB_ITF_TRACE_WHEN_UPDATE_IMPL(_SUFFIX0, _SUFFIX1, _WHEN, _PAYLOAD_WIDTH, _PAYLOAD) \
    `RVB_ITF_TRACE_DECL(_SUFFIX0, _SUFFIX1, _PAYLOAD_WIDTH) \
    `RVB_ITF_TRACE_TICK \
    always @(posedge $root.sim_top.clk) begin \
        if (__itf_trace_enable && (_WHEN) && \
            (__itf_trace_filter == 0 || \
            rvb_itf_trace_filter_eval(__itf_trace_filter, \
                {{(`RVB_ITF_TRACE_FILTER_WIDTH-(_PAYLOAD_WIDTH)){1'b0}}, \
                _PAYLOAD}))) begin \
            `RVB_ITF_TRACE_WRITE; \
        end \
        __itf_trace_update_state(); \
    end

`define RVB_ITF_TRACE_WHEN(_SUFFIX0, _SUFFIX1, _WHEN) \
    `RVB_ITF_TRACE_WHEN_IMPL(_SUFFIX0, _SUFFIX1, _WHEN, $bits(pkt), pkt)

`define RVB_ITF_TRACE_WHEN_UPDATE(_SUFFIX0, _SUFFIX1, _WHEN) \
    `RVB_ITF_TRACE_WHEN_UPDATE_IMPL(_SUFFIX0, _SUFFIX1, _WHEN, \
        $bits(pkt), pkt)

`define RVB_ITF_TRACE_WHEN_NOPKT(_SUFFIX0, _SUFFIX1, _WHEN) \
    `RVB_ITF_TRACE_WHEN_IMPL(_SUFFIX0, _SUFFIX1, _WHEN, 1, 1'b0)

`define RVB_ITF_TRACE_WHEN_UPDATE_NOPKT(_SUFFIX0, _SUFFIX1, _WHEN) \
    `RVB_ITF_TRACE_WHEN_UPDATE_IMPL(_SUFFIX0, _SUFFIX1, _WHEN, 1, 1'b0)

`else
`define RVB_ITF_TRACE_WHEN(_SUFFIX0, _SUFFIX1, _WHEN)
`define RVB_ITF_TRACE_WHEN_UPDATE(_SUFFIX0, _SUFFIX1, _WHEN)
`define RVB_ITF_TRACE_WHEN_NOPKT(_SUFFIX0, _SUFFIX1, _WHEN)
`define RVB_ITF_TRACE_WHEN_UPDATE_NOPKT(_SUFFIX0, _SUFFIX1, _WHEN)
`endif

`endif
