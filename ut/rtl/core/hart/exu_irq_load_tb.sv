`timescale 1ns/1ps

`include "spec/core/isa.svh"

module exu_irq_load_tb;
    logic clk;
    logic rst_n;

    ex_req_if_t ex_req();
    ex_rsp_if_t ex_rsp();
    fl_req_if_t fl_req();
    ldst_req_if_t ldst_req();
    ldst_rsp_if_t ldst_rsp();
    exu_csr_read_req_if_t csr_read_req();
    csr_exu_read_rsp_if_t csr_read_rsp();
    exu_csr_write_req_if_t csr_write_req();
    csr_exu_write_rsp_if_t csr_write_rsp();
    tlb_flush_if_t tlb_flush();
    l1_flush_if_t l1i_flush();
    l1_flush_if_t l1d_flush();
    l1_flush_ack_if_t l1i_flush_ack();
    l1_flush_ack_if_t l1d_flush_ack();
    hart_expt_if_t ex_expt();
    exu_state_if_t exu_state();
    trap_exu_ctrl_if_t trap_ctrl();
    perf_exu_if_t perf();

    exu u_dut(
        .clk                    (clk),
        .rst_n                  (rst_n),
        .ex_req_slv             (ex_req),
        .ex_rsp_mst             (ex_rsp),
        .fl_req_slv             (fl_req),
        .ldst_req_mst           (ldst_req),
        .ldst_rsp_slv           (ldst_rsp),
        .exu_csr_read_req_mst   (csr_read_req),
        .csr_exu_read_rsp_slv   (csr_read_rsp),
        .exu_csr_write_req_mst  (csr_write_req),
        .csr_exu_write_rsp_slv  (csr_write_rsp),
        .tlb_flush_mst          (tlb_flush),
        .l1i_flush_mst          (l1i_flush),
        .l1d_flush_mst          (l1d_flush),
        .l1i_flush_ack_slv      (l1i_flush_ack),
        .l1d_flush_ack_slv      (l1d_flush_ack),
        .ex_expt_mst            (ex_expt),
        .exu_state_mst          (exu_state),
        .trap_exu_ctrl_slv      (trap_ctrl),
        .perf_mst               (perf)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        #100_000;
        $display("FAIL: exu_irq_load_tb watchdog timeout");
        $finish;
    end

    assign ex_rsp.rdy = 1'b1;
    assign ldst_req.rdy = 1'b1;

    function automatic logic [31:0] addi_inst(
        input logic [4:0] rd,
        input logic [4:0] rs1,
        input logic [11:0] imm
    );
        addi_inst = {imm, rs1, 3'b000, rd, 7'b0010011};
    endfunction

    function automatic logic [31:0] lw_inst(
        input logic [4:0] rd,
        input logic [4:0] rs1,
        input logic [11:0] imm
    );
        lw_inst = {imm, rs1, 3'b010, rd, 7'b0000011};
    endfunction

    task automatic tick(input int unsigned count = 1);
        repeat (count) @(posedge clk);
    endtask

    task automatic issue_and_wait(
        input logic [31:0] inst,
        input logic [31:0] pc
    );
        begin
            @(negedge clk);
            ex_req.pkt.inst.raw = inst;
            ex_req.pkt.pc = pc;
            ex_req.pkt.pred_taken = 1'b0;
            ex_req.pkt.pred_pc = pc + 4;
            ex_req.pkt.is_boot_code = 1'b0;
            ex_req.vld = 1'b1;
            do begin
                tick();
            end while (!ex_req.rdy);
            @(negedge clk);
            ex_req.vld = 1'b0;
        end
    endtask

    initial begin
        rst_n = 1'b0;
        ex_req.vld = 1'b0;
        ex_req.pkt = '0;
        fl_req.vld = 1'b0;
        ldst_rsp.vld = 1'b0;
        ldst_rsp.pkt = '0;
        csr_read_rsp.pkt = '0;
        csr_write_rsp.vld = 1'b0;
        csr_write_rsp.pkt = '0;
        l1i_flush_ack.vld = 1'b0;
        l1d_flush_ack.vld = 1'b0;
        trap_ctrl.vld = 1'b0;
        trap_ctrl.pkt = '0;

        tick(5);
        rst_n = 1'b1;
        tick(3);

        if (exu_state.pkt.trap_defer !== 1'b0) begin
            $display("FAIL: empty EXU incorrectly deferred traps");
            $finish;
        end

        issue_and_wait(addi_inst(5'd10, 5'd0, 12'h023), 32'h00010ff8);
        if (u_dut.u_exu_gpr.gprs[10] !== 32'h00000023) begin
            $display("FAIL: a0 initialization got=%08x",
                u_dut.u_exu_gpr.gprs[10]);
            $finish;
        end

        @(negedge clk);
        ex_req.pkt.inst.raw = lw_inst(5'd10, 5'd0, 12'h000);
        ex_req.pkt.pc = 32'h00010ffc;
        ex_req.pkt.pred_taken = 1'b0;
        ex_req.pkt.pred_pc = 32'h00011000;
        ex_req.pkt.is_boot_code = 1'b0;
        ex_req.vld = 1'b1;
        do begin
            tick();
        end while (!(ldst_req.vld && ldst_req.rdy));

        tick(2);

        if (exu_state.pkt.trap_defer !== 1'b1) begin
            $display("FAIL: outstanding load did not defer traps");
            $finish;
        end

        @(negedge clk);
        ldst_rsp.pkt.data = 32'h0009348c;
        ldst_rsp.pkt.ok = 1'b1;
        ldst_rsp.vld = 1'b1;
        trap_ctrl.pkt.priv = 2'b00;
        trap_ctrl.pkt.irq_epc = 32'h00011000;
        trap_ctrl.pkt.wfi = 1'b1;
        trap_ctrl.vld = 1'b1;
        tick();
        @(negedge clk);
        ex_req.vld = 1'b0;
        ldst_rsp.vld = 1'b0;
        trap_ctrl.vld = 1'b0;
        tick(2);

        if (u_dut.u_exu_gpr.gprs[10] !== 32'h0009348c) begin
            $display("FAIL: boundary trap suppressed retiring load a0=%08x",
                u_dut.u_exu_gpr.gprs[10]);
            $finish;
        end

        @(negedge clk);
        trap_ctrl.pkt.priv = 2'b00;
        trap_ctrl.pkt.irq_epc = 32'h00011000;
        trap_ctrl.pkt.wfi = 1'b0;
        trap_ctrl.vld = 1'b1;
        tick();
        @(negedge clk);
        trap_ctrl.vld = 1'b0;

        @(negedge clk);
        ex_req.pkt.inst.raw = lw_inst(5'd15, 5'd10, 12'h008);
        ex_req.pkt.pc = 32'h00011008;
        ex_req.pkt.pred_taken = 1'b0;
        ex_req.pkt.pred_pc = 32'h0001100c;
        ex_req.pkt.is_boot_code = 1'b0;
        ex_req.vld = 1'b1;
        do begin
            tick();
        end while (!ldst_req.vld);

        if (ldst_req.pkt.addr !== 32'h00093494) begin
            $display("FAIL: post-IRQ load used stale a0 addr=%08x a0=%08x",
                ldst_req.pkt.addr, u_dut.u_exu_gpr.gprs[10]);
            $finish;
        end

        $display("PASS: exu_irq_load_tb");
        $finish;
    end
endmodule
