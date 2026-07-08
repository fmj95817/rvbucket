module gtimer(
    input logic      clk,
    input logic      rst_n,
    apb_req_if_t.slv apb_req_slv,
    apb_rsp_if_t.mst apb_rsp_mst,
    ext_irq_if_t.mst irq_mst
);
    localparam BASE = 32'h30002000;
    localparam REG_CTRL = 2'd0;
    localparam REG_COUNT = 2'd1;
    localparam REG_RELOAD = 2'd2;
    localparam REG_IRQ = 2'd3;

    logic pending;
    logic [31:0] ctrl;
    logic [31:0] count;
    logic [31:0] reload;
    logic irq_pend;
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic [31:0] rsp_data_dec;
    logic rsp_ok_dec;
    logic ctrl_wen;
    logic reload_wen;
    logic irq_clr;

    wire req_hsk = apb_req_slv.psel && apb_req_slv.penable && !pending;
    wire rsp_hsk = pending;
    wire req_write = apb_req_slv.pkt.pwrite;
    wire [31:0] offset = apb_req_slv.pkt.paddr - BASE;
    wire [1:0] reg_addr = offset[3:2];

    always_comb begin
        rsp_data_dec = '0;
        rsp_ok_dec = 1'b1;
        ctrl_wen = 1'b0;
        reload_wen = 1'b0;
        irq_clr = 1'b0;

        unique case (reg_addr)
        REG_CTRL: begin
            if (req_write)
                ctrl_wen = 1'b1;
            else
                rsp_data_dec = ctrl;
        end
        REG_COUNT: begin
            if (!req_write)
                rsp_data_dec = count;
        end
        REG_RELOAD: begin
            if (req_write)
                reload_wen = 1'b1;
            else
                rsp_data_dec = reload;
        end
        REG_IRQ: begin
            if (req_write)
                irq_clr = apb_req_slv.pkt.pwdata[0];
            else
                rsp_data_dec = {31'b0, irq_pend};
        end
        default: begin
            rsp_ok_dec = 1'b0;
        end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            ctrl <= '0;
            count <= '0;
            reload <= '0;
            irq_pend <= 1'b0;
            rsp_data <= '0;
            rsp_ok <= 1'b1;
        end else begin
            if (ctrl[0] && count != 0) begin
                count <= count - 1'b1;
                if (count == 1)
                    irq_pend <= 1'b1;
            end

            if (req_hsk) begin
                pending <= 1'b1;
                rsp_data <= rsp_data_dec;
                rsp_ok <= rsp_ok_dec;

                if (ctrl_wen)
                    ctrl <= {31'b0, apb_req_slv.pkt.pwdata[0]};
                if (reload_wen) begin
                    reload <= apb_req_slv.pkt.pwdata;
                    count <= apb_req_slv.pkt.pwdata;
                    irq_pend <= 1'b0;
                end
                if (irq_clr)
                    irq_pend <= 1'b0;
            end else if (rsp_hsk) begin
                pending <= 1'b0;
            end
        end
    end

    assign apb_rsp_mst.pready = pending;
    assign apb_rsp_mst.pkt.prdata = rsp_data;
    assign apb_rsp_mst.pkt.pslverr = !rsp_ok;
    assign irq_mst.pkt.irq = irq_pend;
endmodule
