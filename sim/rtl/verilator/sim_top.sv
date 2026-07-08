`include "spec/core/isa.svh"

module sim_top(
    input logic clk,
    input logic rst_n
);
    logic uart_tx;
    logic uart_rx;
    logic uart_rx_ch_vld;
    logic [7:0] uart_rx_ch;
    logic itf_dump_en;
    logic progress_en;
    logic user_trace_en;
    logic fast_linux_en;
    int unsigned itf_dump_count;
    int unsigned itf_dump_cycle;
    int unsigned kmem_trace_left;
    int unsigned user_trace_cycle;
    int unsigned user_trace_inst_count;
    assign uart_rx = 1'b1;

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

    always @(negedge clk) begin
        if (uart_rx_ch_vld) begin
            if (uart_rx_ch != 8'h10)
                $write("%c", uart_rx_ch);
            else
                $finish;
        end
    end

    always @(posedge clk) begin
        if (progress_en && rst_n) begin
            itf_dump_cycle <= itf_dump_cycle + 1;
            if (u_soc.u_rv32g.u_hart.ex_req_if.vld && u_soc.u_rv32g.u_hart.ex_req_if.rdy &&
                u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc == 32'hc00e9be8 &&
                itf_dump_cycle > 297700000)
                kmem_trace_left <= 200;
            else if (kmem_trace_left != 0 &&
                u_soc.u_rv32g.u_hart.ex_req_if.vld && u_soc.u_rv32g.u_hart.ex_req_if.rdy)
                kmem_trace_left <= kmem_trace_left - 1;
            if ((kmem_trace_left != 0 ||
                (u_soc.u_rv32g.u_hart.ex_req_if.vld && u_soc.u_rv32g.u_hart.ex_req_if.rdy &&
                 u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc == 32'hc00e9be8 &&
                 itf_dump_cycle > 297700000)) &&
                u_soc.u_rv32g.u_hart.ex_req_if.vld &&
                u_soc.u_rv32g.u_hart.ex_req_if.rdy) begin
                $display("KMEM_SEQ_TRACE cycle=%0d pc=%08x ex_rdy=%0b raw=%08x opcode=%02x x1=%08x x5=%08x x6=%08x x10=%08x x11=%08x x12=%08x x13=%08x x14=%08x x15=%08x br_rs1=%08x target=%08x taken=%0b ifu_state=%0d wfi=%0b irqv=%0b",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc,
                    u_soc.u_rv32g.u_hart.ex_req_if.rdy,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.base.opcode,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[1],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[5],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[6],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[10],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[11],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[12],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[13],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[14],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[15],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_branch_handler.gpr_mst.rd1,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_branch_handler.ex_rsp_mst.pkt.target_pc,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_branch_handler.ex_rsp_mst.pkt.taken,
                    u_soc.u_rv32g.u_hart.u_ifu.state,
                    u_soc.u_rv32g.u_hart.u_exu.wfi,
                    u_soc.u_rv32g.u_hart.u_trap.irq_valid);
            end
            if (u_soc.u_rv32g.u_hart.ex_req_if.vld &&
                u_soc.u_rv32g.u_hart.ex_req_if.rdy &&
                itf_dump_cycle > 473360000 && itf_dump_cycle < 473413000 &&
                u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc >= 32'hc011a58c &&
                u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc <= 32'hc011a5c4) begin
                $display("DLOOKUP_TRACE cycle=%0d pc=%08x raw=%08x x1=%08x x10=%08x x11=%08x x12=%08x x13=%08x x14=%08x x15=%08x x16=%08x x28=%08x x30=%08x x31=%08x gpr_wen=%0b gpr_wa=%0d gpr_wd=%08x ldst_state=%0d mmu_state=%0d va=%08x pa_req=%0b/%0b pa_rsp=%0b/%0b pa_data=%08x irqv=%0b",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[1],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[10],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[11],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[12],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[13],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[14],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[15],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[16],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[28],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[30],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[31],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gpr_slv.wen,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gpr_slv.wa,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gpr_slv.wd,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_ldst_handler.state,
                    u_soc.u_rv32g.u_hart.u_mmu.state,
                    u_soc.u_rv32g.u_hart.u_mmu.va,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.rdy,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.rdy,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.pkt.data,
                    u_soc.u_rv32g.u_hart.u_trap.irq_valid);
            end
            if (u_soc.u_rv32g.u_hart.ex_req_if.vld &&
                u_soc.u_rv32g.u_hart.ex_req_if.rdy &&
                itf_dump_cycle > 473360000 && itf_dump_cycle < 473413000 &&
                (u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc == 32'hc02e7f04 ||
                 u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc == 32'hc02e8030 ||
                 u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc == 32'hc02e8074)) begin
                $display("TRAPREG_TRACE cycle=%0d pc=%08x raw=%08x x2=%08x x16=%08x gpr_wen=%0b gpr_wa=%0d gpr_wd=%08x sepc=%08x sstatus=%08x scause=%08x stval=%08x ldst_state=%0d va=%08x pa_rsp=%0b/%0b pa_data=%08x",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[2],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[16],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gpr_slv.wen,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gpr_slv.wa,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gpr_slv.wd,
                    u_soc.u_rv32g.u_hart.u_csr.csr_sepc,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mstatus,
                    u_soc.u_rv32g.u_hart.u_csr.csr_scause,
                    u_soc.u_rv32g.u_hart.u_csr.csr_stval,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_ldst_handler.state,
                    u_soc.u_rv32g.u_hart.u_mmu.va,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.rdy,
                    u_soc.u_rv32g.u_hart.pa_d_bti_rsp_if.pkt.data);
            end
            if (itf_dump_cycle % 1000000 == 0)
                $display("PROGRESS cycle=%0d pc=%08x state=%0d priv=%0d wfi=%0b irqv=%0b mip=%08x mie=%08x mstatus=%08x time=%08x_%08x stcmp=%08x_%08x mmu=%0d va=%08x l1d=%0d pa_d_req=%0b/%0b d_ar=%0b/%0b l2_rd=%0b/%0b mm_ar=%0b/%0b ddr_ar=%0b/%0b d_r=%0b/%0b",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.u_ifu.pc,
                    u_soc.u_rv32g.u_hart.u_ifu.state,
                    u_soc.u_rv32g.u_hart.u_exu.priv,
                    u_soc.u_rv32g.u_hart.u_exu.wfi,
                    u_soc.u_rv32g.u_hart.u_trap.irq_valid,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mip,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mie,
                    u_soc.u_rv32g.u_hart.u_csr.csr_mstatus,
                    u_soc.u_rv32g.u_hart.u_csr.csr_timeh,
                    u_soc.u_rv32g.u_hart.u_csr.csr_time,
                    u_soc.u_rv32g.u_hart.u_csr.csr_stimecmph,
                    u_soc.u_rv32g.u_hart.u_csr.csr_stimecmp,
                    u_soc.u_rv32g.u_hart.u_mmu.state,
                    u_soc.u_rv32g.u_hart.u_mmu.va,
                    u_soc.u_rv32g.u_hart.u_l1d.u_bti2axi.state,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.rdy,
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
        if (user_trace_en && rst_n) begin
            user_trace_cycle <= user_trace_cycle + 1;
            if ((user_trace_cycle % 20000000) == 0) begin
                $display("USER_PROGRESS cycle=%0d pc=%08x raw=%08x ifu_state=%0d priv=%0d wfi=%0b trap_state=%0d irqv=%0b mip=%08x mie=%08x mstatus=%08x sepc=%08x scause=%08x stval=%08x mmu=%0d va=%08x l1d=%0d pa_d_req=%0b/%0b d_ar=%0b/%0b l2_rd=%0b/%0b mm_ar=%0b/%0b ddr_ar=%0b/%0b d_r=%0b/%0b a0=%08x a1=%08x a2=%08x a3=%08x a4=%08x a5=%08x a7=%08x",
                    user_trace_cycle,
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
                    u_soc.u_rv32g.u_hart.u_csr.csr_sepc,
                    u_soc.u_rv32g.u_hart.u_csr.csr_scause,
                    u_soc.u_rv32g.u_hart.u_csr.csr_stval,
                    u_soc.u_rv32g.u_hart.u_mmu.state,
                    u_soc.u_rv32g.u_hart.u_mmu.va,
                    u_soc.u_rv32g.u_hart.u_l1d.u_bti2axi.state,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.pa_d_bti_req_if.rdy,
                    u_soc.u_rv32g.hart_d_ar.vld,
                    u_soc.u_rv32g.hart_d_ar.rdy,
                    u_soc.u_rv32g.u_l2.rd_active,
                    u_soc.u_rv32g.u_l2.rd_active_d,
                    u_soc.mm_ar.vld,
                    u_soc.mm_ar.rdy,
                    u_soc.mm_gst_ar[0].vld,
                    u_soc.mm_gst_ar[0].rdy,
                    u_soc.u_rv32g.hart_d_r.vld,
                    u_soc.u_rv32g.hart_d_r.rdy,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[10],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[11],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[12],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[13],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[14],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[15],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[17]);
            end
            if (u_soc.u_rv32g.u_hart.ex_req_if.vld &&
                u_soc.u_rv32g.u_hart.ex_req_if.rdy &&
                u_soc.u_rv32g.u_hart.u_exu.priv == 2'b00) begin
                if (user_trace_inst_count < 200 ||
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw == 32'h00000073) begin
                    $display("USER_EX cycle=%0d idx=%0d pc=%08x raw=%08x a0=%08x a1=%08x a2=%08x a3=%08x a4=%08x a5=%08x a7=%08x sp=%08x ra=%08x",
                        user_trace_cycle,
                        user_trace_inst_count,
                        u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc,
                        u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw,
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[10],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[11],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[12],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[13],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[14],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[15],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[17],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[2],
                        u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[1]);
                end
                user_trace_inst_count <= user_trace_inst_count + 1;
            end
            if (u_soc.u_rv32g.u_hart.trap_csr_write_req_if.vld &&
                u_soc.u_rv32g.u_hart.trap_csr_write_req_if.pkt.addr == 12'h142 &&
                !u_soc.u_rv32g.u_hart.trap_csr_write_req_if.pkt.val[31]) begin
                $display("USER_TRAP cycle=%0d scause=%08x sepc=%08x stval=%08x priv=%0d pc=%08x raw=%08x a0=%08x a1=%08x a2=%08x a7=%08x",
                    user_trace_cycle,
                    u_soc.u_rv32g.u_hart.trap_csr_write_req_if.pkt.val,
                    u_soc.u_rv32g.u_hart.u_csr.csr_sepc,
                    u_soc.u_rv32g.u_hart.u_csr.csr_stval,
                    u_soc.u_rv32g.u_hart.u_exu.priv,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.pc,
                    u_soc.u_rv32g.u_hart.ex_req_if.pkt.inst.raw,
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[10],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[11],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[12],
                    u_soc.u_rv32g.u_hart.u_exu.u_exu_gpr.gprs[17]);
            end
        end
        if (itf_dump_en && rst_n) begin
            itf_dump_cycle <= itf_dump_cycle + 1;
            if (itf_dump_cycle < 20 || itf_dump_cycle % 1000 == 0)
                $display("ITF STATUS cycle=%0d i_req=%0b/%0b i_ar=%0b/%0b d_req=%0b/%0b",
                    itf_dump_cycle,
                    u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.rdy,
                    u_soc.u_rv32g.hart_i_ar.vld,
                    u_soc.u_rv32g.hart_i_ar.rdy,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.vld,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.rdy);
            if (u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.rdy) begin
                $display("ITF I_REQ addr=%08x", u_soc.u_rv32g.u_hart.hbi_i_bti_req_if.pkt.addr);
                itf_dump_count <= itf_dump_count + 1;
            end
            if (u_soc.u_rv32g.u_hart.hbi_i_bti_rsp_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_i_bti_rsp_if.rdy)
                $display("ITF I_RSP data=%08x", u_soc.u_rv32g.u_hart.hbi_i_bti_rsp_if.pkt.data);
            if (u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.rdy) begin
                $display("ITF D_REQ cmd=%0d addr=%08x data=%08x strb=%x",
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.cmd,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.addr,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.data,
                    u_soc.u_rv32g.u_hart.hbi_d_bti_req_if.pkt.strobe);
                itf_dump_count <= itf_dump_count + 1;
            end
            if (u_soc.u_rv32g.u_hart.hbi_d_bti_rsp_if.vld &&
                u_soc.u_rv32g.u_hart.hbi_d_bti_rsp_if.rdy)
                $display("ITF D_RSP data=%08x", u_soc.u_rv32g.u_hart.hbi_d_bti_rsp_if.pkt.data);
            if (itf_dump_count >= 500 || itf_dump_cycle >= 10000) begin
                $display("ITF dump limit reached");
                $finish;
            end
        end
    end

    initial begin
        string path;
        itf_dump_en = $test$plusargs("itf_dump");
        progress_en = $test$plusargs("progress");
        user_trace_en = $test$plusargs("user_trace");
        itf_dump_count = 0;
        itf_dump_cycle = 0;
        kmem_trace_left = 0;
        user_trace_cycle = 0;
        user_trace_inst_count = 0;
        if ($value$plusargs("program=%s", path)) begin
            $readmemh(path, u_soc.u_flash.u_rom.mem);
        end
        fast_linux_en = $test$plusargs("fast_linux");
        if ($test$plusargs("vcd")) begin
            $dumpfile("sim_top.vcd");
            $dumpvars(0, sim_top);
        end
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
