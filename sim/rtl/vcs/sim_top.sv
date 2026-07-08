module sim_top;
    logic clk;
    logic rst_n;

    logic uart_tx;
    logic uart_rx;
    logic uart_rx_ch_vld;
    logic [7:0] uart_rx_ch;
    logic fast_linux_en;
    logic progress_en;
    longint unsigned progress_cycle;
    assign uart_rx = 1'b1;

    clk_rst u_clk_rst(
        .clk     (clk),
        .rst_n   (rst_n)
    );

    soc u_soc(
        .clk     (clk),
        .rst_n   (rst_n),
        .uart_tx (uart_tx),
        .uart_rx (uart_rx)
    );

    uart_rx u_uart_rx(
        .clk     (clk),
        .rst_n   (rst_n),
        .bc      (16'd9),
        .ch_vld  (uart_rx_ch_vld),
        .ch      (uart_rx_ch),
        .rx      (uart_tx)
    );

    always @(posedge clk) begin
        if (uart_rx_ch_vld) begin
            if (uart_rx_ch != 8'h10)
                $write("%c", uart_rx_ch);
            else
                $finish;
        end
    end

    always @(posedge clk) begin
        if (progress_en && rst_n) begin
            progress_cycle <= progress_cycle + 1;
            if ((progress_cycle % 10000000) == 0) begin
                $display("VCS_PROGRESS cycle=%0d pc=%08x raw=%08x ifu_state=%0d priv=%0d wfi=%0b trap_state=%0d irqv=%0b mip=%08x mie=%08x mstatus=%08x mmu=%0d va=%08x l1d=%0d pa_d_req=%0b/%0b pa_d_rsp=%0b/%0b d_ar=%0b/%0b l2_rd=%0b/%0b mm_ar=%0b/%0b ddr_ar=%0b/%0b d_r=%0b/%0b",
                    progress_cycle,
                    u_soc.u_rv32g.u_hart.u_ifu.pc,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw,
                    u_soc.u_rv32g.u_hart.u_ifu.state,
                    u_soc.u_rv32g.u_hart.u_exu.priv,
                    u_soc.u_rv32g.u_hart.u_exu.wfi,
                    u_soc.u_rv32g.u_hart.u_trap.state,
                    u_soc.u_rv32g.u_hart.u_trap.irq_valid,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mip,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mie,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mstatus,
                    u_soc.u_rv32g.u_hart.u_mmu.state,
                    u_soc.u_rv32g.u_hart.u_mmu.va,
                    u_soc.u_rv32g.u_hart.u_l1d.u_bti2axi.state,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.rdy,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.rdy,
                    u_soc.u_rv32g.hart_d_ar.vld,
                    u_soc.u_rv32g.hart_d_ar.rdy,
                    u_soc.u_rv32g.u_l2.rd_active,
                    u_soc.u_rv32g.u_l2.rd_active_d,
                    u_soc.mm_ar.vld,
                    u_soc.mm_ar.rdy,
                    u_soc.mm_gst_ar[0].vld,
                    u_soc.mm_gst_ar[0].rdy,
                    u_soc.u_rv32g.hart_d_r.vld,
                    u_soc.u_rv32g.hart_d_r.rdy);
            end
        end
    end

    initial begin
        string path;
        if ($value$plusargs("program=%s", path)) begin
            $readmemh(path, u_soc.u_flash.u_rom.mem);
        end
        fast_linux_en = $test$plusargs("fast_linux");
        progress_en = $test$plusargs("progress");
        progress_cycle = 0;
`ifdef FSDB
        if ($test$plusargs("fsdb")) begin
            $fsdbDumpfile("sim_top.fsdb");
            $fsdbDumpvars(0, sim_top, "+all");
        end
`endif
    end

    always @(negedge rst_n) begin
        if (fast_linux_en) begin
            $readmemh("../../sw/linux/kernel.0.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank0.mem, 0);
            $readmemh("../../sw/linux/kernel.1.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank1.mem, 0);
            $readmemh("../../sw/linux/kernel.2.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank2.mem, 0);
            $readmemh("../../sw/linux/kernel.3.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank3.mem, 0);
            $readmemh("../../sw/linux/initrd.0.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank0.mem, 26'h1400000);
            $readmemh("../../sw/linux/initrd.1.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank1.mem, 26'h1400000);
            $readmemh("../../sw/linux/initrd.2.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank2.mem, 26'h1400000);
            $readmemh("../../sw/linux/initrd.3.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank3.mem, 26'h1400000);
            $readmemh("../../sw/linux/dtb.0.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank0.mem, 26'h13c0000);
            $readmemh("../../sw/linux/dtb.1.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank1.mem, 26'h13c0000);
            $readmemh("../../sw/linux/dtb.2.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank2.mem, 26'h13c0000);
            $readmemh("../../sw/linux/dtb.3.hex", u_soc.u_ddr.u_bti_sram.u_sram_bank3.mem, 26'h13c0000);
        end
    end
endmodule
