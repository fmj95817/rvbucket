module aclint(
    input logic           clk,
    input logic           rst_n,
    apb_req_if_t.slv      apb_req_slv,
    apb_rsp_if_t.mst      apb_rsp_mst,
    core_timer_if_t.mst   core_timer_mst,
    core_m_irq_if_t.mst   core_m_irq_mst
);
    localparam MTIMER_TICK_CYCLES = 10000;

    logic [63:0] mtime;
    logic [63:0] mtimecmp;
    logic [$clog2(MTIMER_TICK_CYCLES)-1:0] tick_count;
    logic pending;
    logic [31:0] rsp_data;
    logic rsp_err;
    logic [31:0] rsp_data_dec;
    logic rsp_err_dec;
    logic mtime_lo_wen;
    logic mtime_hi_wen;
    logic mtimecmp_lo_wen;
    logic mtimecmp_hi_wen;

    wire req_hsk = apb_req_slv.psel && apb_req_slv.penable && !pending;
    wire req_write = apb_req_slv.pkt.pwrite;

    always_comb begin
        rsp_data_dec = '0;
        rsp_err_dec = 1'b0;
        mtime_lo_wen = 1'b0;
        mtime_hi_wen = 1'b0;
        mtimecmp_lo_wen = 1'b0;
        mtimecmp_hi_wen = 1'b0;

        unique case (apb_req_slv.pkt.paddr)
        32'h31000000: begin
            rsp_data_dec = mtime[31:0];
            mtime_lo_wen = req_write;
        end
        32'h31000004: begin
            rsp_data_dec = mtime[63:32];
            mtime_hi_wen = req_write;
        end
        32'h31010000: begin
            rsp_data_dec = mtimecmp[31:0];
            mtimecmp_lo_wen = req_write;
        end
        32'h31010004: begin
            rsp_data_dec = mtimecmp[63:32];
            mtimecmp_hi_wen = req_write;
        end
        default: begin
            rsp_err_dec = 1'b1;
        end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            mtime <= 0;
            mtimecmp <= 64'hffffffffffffffff;
            tick_count <= 0;
            pending <= 0;
            rsp_data <= 0;
            rsp_err <= 1'b0;
        end else begin
            tick_count <= tick_count + 1'b1;
            if (tick_count == MTIMER_TICK_CYCLES - 1) begin
                tick_count <= 0;
                mtime <= mtime + 1'b1;
            end
            if (req_hsk) begin
                pending <= 1'b1;
                rsp_data <= rsp_data_dec;
                rsp_err <= rsp_err_dec;

                if (mtime_lo_wen)
                    mtime[31:0] <= apb_req_slv.pkt.pwdata;
                if (mtime_hi_wen)
                    mtime[63:32] <= apb_req_slv.pkt.pwdata;
                if (mtimecmp_lo_wen)
                    mtimecmp[31:0] <= apb_req_slv.pkt.pwdata;
                if (mtimecmp_hi_wen)
                    mtimecmp[63:32] <= apb_req_slv.pkt.pwdata;
            end else if (pending) begin
                pending <= 1'b0;
            end
        end
    end

    assign apb_rsp_mst.pready = pending;
    assign apb_rsp_mst.pkt.prdata = rsp_data;
    assign apb_rsp_mst.pkt.pslverr = rsp_err;
    assign core_timer_mst.pkt.mtime = mtime;
    assign core_m_irq_mst.pkt.mtimer = mtime >= mtimecmp;
    assign core_m_irq_mst.pkt.msw = 1'b0;
endmodule
