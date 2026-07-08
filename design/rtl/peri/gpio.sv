module gpio(
    input logic         clk,
    input logic         rst_n,
    apb_req_if_t.slv    apb_req_slv,
    apb_rsp_if_t.mst    apb_rsp_mst,
    input logic [23:0]  gpio_in,
    output logic [23:0] gpio_out,
    output logic [23:0] gpio_oe,
    ext_irq_if_t.mst    irq_mst
);
    localparam BASE = 32'h30001000;
    localparam REG_OUT = 2'd0;
    localparam REG_MODE_LO = 2'd1;
    localparam REG_MODE_HI = 2'd2;
    localparam GPIO_MODE_OUT = 2'd0;
    localparam GPIO_MODE_IN = 2'd1;
    localparam GPIO_MODE_IN_IRQ = 2'd2;

    logic pending;
    logic [23:0] output_val;
    logic [31:0] mode_lo;
    logic [31:0] mode_hi;
    logic [23:0] gpio_in_d;
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic [31:0] rsp_data_dec;
    logic rsp_ok_dec;
    logic output_val_wen;
    logic mode_lo_wen;
    logic mode_hi_wen;
    logic [23:0] output_val_wdata;
    logic [31:0] mode_lo_wdata;
    logic [31:0] mode_hi_wdata;

    wire req_hsk = apb_req_slv.psel && apb_req_slv.penable && !pending;
    wire rsp_hsk = pending;
    wire [31:0] offset = apb_req_slv.pkt.paddr - BASE;
    wire [1:0] reg_addr = offset[3:2];
    wire req_write = apb_req_slv.pkt.pwrite;

    function automatic logic [1:0] pin_mode(input logic [4:0] pin);
        if (pin < 5'd16)
            pin_mode = mode_lo[pin[3:0] * 2 +: 2];
        else
            pin_mode = mode_hi[pin[3:0] * 2 +: 2];
    endfunction

    function automatic logic [23:0] read_gpio_val;
        logic [23:0] val;
        val = output_val;
        for (int pin = 0; pin < 24; pin++) begin
            if (pin_mode(pin[4:0]) == GPIO_MODE_IN ||
                pin_mode(pin[4:0]) == GPIO_MODE_IN_IRQ)
                val[pin] = gpio_in[pin];
        end
        read_gpio_val = val;
    endfunction

    always_comb begin
        gpio_oe = '0;
        for (int pin = 0; pin < 24; pin++)
            gpio_oe[pin] = pin_mode(pin[4:0]) == GPIO_MODE_OUT;
    end

    assign gpio_out = output_val & gpio_oe;

    always_comb begin
        rsp_data_dec = '0;
        rsp_ok_dec = 1'b1;
        output_val_wen = 1'b0;
        mode_lo_wen = 1'b0;
        mode_hi_wen = 1'b0;
        output_val_wdata = apb_req_slv.pkt.pwdata[23:0];
        mode_lo_wdata = apb_req_slv.pkt.pwdata;
        mode_hi_wdata = {16'b0, apb_req_slv.pkt.pwdata[15:0]};

        unique case (reg_addr)
        REG_OUT: begin
            if (req_write)
                output_val_wen = 1'b1;
            else
                rsp_data_dec = {8'b0, read_gpio_val()};
        end
        REG_MODE_LO: begin
            if (req_write)
                mode_lo_wen = 1'b1;
            else
                rsp_data_dec = mode_lo;
        end
        REG_MODE_HI: begin
            if (req_write)
                mode_hi_wen = 1'b1;
            else
                rsp_data_dec = mode_hi;
        end
        default: begin
            rsp_ok_dec = 1'b0;
        end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
            output_val <= '0;
            mode_lo <= '0;
            mode_hi <= '0;
            gpio_in_d <= '0;
            rsp_data <= '0;
            rsp_ok <= 1'b1;
        end else begin
            gpio_in_d <= gpio_in;

            if (req_hsk) begin
                pending <= 1'b1;
                rsp_data <= rsp_data_dec;
                rsp_ok <= rsp_ok_dec;

                if (output_val_wen)
                    output_val <= output_val_wdata;
                if (mode_lo_wen)
                    mode_lo <= mode_lo_wdata;
                if (mode_hi_wen)
                    mode_hi <= mode_hi_wdata;
            end else if (rsp_hsk) begin
                pending <= 1'b0;
            end
        end
    end

    assign apb_rsp_mst.pready = pending;
    assign apb_rsp_mst.pkt.prdata = rsp_data;
    assign apb_rsp_mst.pkt.pslverr = !rsp_ok;
    always_comb begin
        irq_mst.pkt.irq = 1'b0;
        for (int pin = 0; pin < 24; pin++) begin
            if (pin_mode(pin[4:0]) == GPIO_MODE_IN_IRQ &&
                gpio_in[pin] != gpio_in_d[pin])
                irq_mst.pkt.irq = 1'b1;
        end
    end
endmodule
