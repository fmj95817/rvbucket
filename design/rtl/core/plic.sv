module plic(
    input logic        clk,
    input logic        rst_n,
    apb_req_if_t.slv   apb_req_slv,
    apb_rsp_if_t.mst   apb_rsp_mst,
    ext_irq_if_t.slv   uart_irq_slv,
    ext_irq_if_t.slv   gpio_irq_slv,
    ext_irq_if_t.slv   gtimer_irq_slv,
    ext_irq_if_t.slv   sdspi_irq_slv,
    ext_irq_if_t.mst   core_irq_mst
);
    localparam SOURCE_NUM = 33;
    localparam ACTIVE_SOURCE_NUM = 5;
    localparam BASE = 32'h31100000;

    logic [2:0] priorities[0:SOURCE_NUM-1];
    logic [SOURCE_NUM-1:0] pending;
    logic [SOURCE_NUM-1:0] claimed;
    logic [SOURCE_NUM-1:0] enable;
    logic [2:0] threshold;
    logic rsp_pending;
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic [31:0] rsp_data_dec;
    logic rsp_ok_dec;
    logic priority_wen;
    logic [5:0] priority_waddr;
    logic [2:0] priority_wdata;
    logic enable_lo_wen;
    logic enable_hi_wen;
    logic threshold_wen;
    logic complete_wen;
    logic [5:0] complete_source;
    logic claim_ren;

    logic [5:0] best_source;
    logic [5:0] best_source_nxt;
    logic [2:0] best_priority_nxt;

    always_comb begin
        best_source_nxt = 0;
        best_priority_nxt = threshold;
        for (int source = 1; source < ACTIVE_SOURCE_NUM; source++) begin
            if (pending[source] && !claimed[source] && enable[source] &&
                priorities[source] > best_priority_nxt) begin
                best_source_nxt = source[5:0];
                best_priority_nxt = priorities[source];
            end
        end
    end

    wire req_hsk = apb_req_slv.psel && apb_req_slv.penable && !rsp_pending;
    wire rsp_hsk = rsp_pending;
    wire req_write = apb_req_slv.pkt.pwrite;
    wire [31:0] offset = apb_req_slv.pkt.paddr - BASE;
    integer i;

    always_comb begin
        rsp_data_dec = '0;
        rsp_ok_dec = 1'b1;
        priority_wen = 1'b0;
        priority_waddr = offset[7:2];
        priority_wdata = apb_req_slv.pkt.pwdata[2:0];
        enable_lo_wen = 1'b0;
        enable_hi_wen = 1'b0;
        threshold_wen = 1'b0;
        complete_wen = 1'b0;
        complete_source = apb_req_slv.pkt.pwdata[5:0];
        claim_ren = 1'b0;

        if (offset < SOURCE_NUM * 4) begin
            if (req_write && offset[31:2] != 0)
                priority_wen = 1'b1;
            else if (!req_write)
                rsp_data_dec = {29'b0, priorities[offset[7:2]]};
        end else if (offset == 32'h00001000) begin
            if (!req_write)
                rsp_data_dec = pending[31:0] & 32'hfffffffe;
        end else if (offset == 32'h00001004) begin
            if (!req_write)
                rsp_data_dec = {31'b0, pending[32]};
        end else if (offset == 32'h00002000) begin
            if (req_write)
                enable_lo_wen = 1'b1;
            else
                rsp_data_dec = enable[31:0];
        end else if (offset == 32'h00002004) begin
            if (req_write)
                enable_hi_wen = 1'b1;
            else
                rsp_data_dec = {31'b0, enable[32]};
        end else if (offset == 32'h00200000) begin
            if (req_write)
                threshold_wen = 1'b1;
            else
                rsp_data_dec = {29'b0, threshold};
        end else if (offset == 32'h00200004) begin
            if (req_write) begin
                complete_wen = apb_req_slv.pkt.pwdata > 0 &&
                    apb_req_slv.pkt.pwdata < SOURCE_NUM;
            end else begin
                rsp_data_dec = {26'b0, best_source};
                claim_ren = best_source != 0;
            end
        end else begin
            rsp_ok_dec = 1'b0;
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < SOURCE_NUM; i++) priorities[i] <= 0;
            pending <= 0;
            claimed <= 0;
            enable <= 0;
            threshold <= 0;
            best_source <= 0;
            rsp_pending <= 0;
            rsp_data <= 0;
            rsp_ok <= 1'b1;
        end else begin
            best_source <= best_source_nxt;

            if (uart_irq_slv.pkt.irq && !claimed[1]) pending[1] <= 1'b1;
            if (gpio_irq_slv.pkt.irq && !claimed[2]) pending[2] <= 1'b1;
            if (gtimer_irq_slv.pkt.irq && !claimed[3]) pending[3] <= 1'b1;
            if (sdspi_irq_slv.pkt.irq && !claimed[4]) pending[4] <= 1'b1;
            if (rsp_hsk) rsp_pending <= 1'b0;

            if (req_hsk) begin
                rsp_pending <= 1'b1;
                rsp_data <= rsp_data_dec;
                rsp_ok <= rsp_ok_dec;

                if (priority_wen)
                    priorities[priority_waddr] <= priority_wdata;
                if (enable_lo_wen)
                    enable[31:0] <= apb_req_slv.pkt.pwdata & 32'hfffffffe;
                if (enable_hi_wen)
                    enable[32] <= apb_req_slv.pkt.pwdata[0];
                if (threshold_wen)
                    threshold <= apb_req_slv.pkt.pwdata[2:0];
                if (complete_wen)
                    claimed[complete_source] <= 1'b0;
                if (claim_ren) begin
                    pending[best_source] <= 1'b0;
                    claimed[best_source] <= 1'b1;
                end
            end
        end
    end

    assign apb_rsp_mst.pready = rsp_pending;
    assign apb_rsp_mst.pkt.prdata = rsp_data;
    assign apb_rsp_mst.pkt.pslverr = !rsp_ok;
    assign core_irq_mst.pkt.irq = best_source != 0;
endmodule
