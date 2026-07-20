`timescale 1ns/1ps

`include "itf/hart_expt_if.svh"

module trap_expt_priority_tb;
    logic clk;
    logic rst_n;
    logic saw_scause;
    logic saw_sepc;
    logic saw_stval;

    hart_expt_if_t ex_expt();
    hart_expt_if_t fch_expt();
    hart_expt_if_t ldst_expt();
    exu_state_if_t exu_state();
    trap_exu_ctrl_if_t trap_exu_ctrl();
    csr_trap_state_if_t csr_trap_state();
    trap_csr_write_req_if_t trap_csr_write_req();
    csr_trap_write_rsp_if_t csr_trap_write_rsp();
    trap_send_if_t trap_send();

    trap u_dut(
        .clk                    (clk),
        .rst_n                  (rst_n),
        .ex_expt_slv            (ex_expt),
        .fch_expt_slv           (fch_expt),
        .ldst_expt_slv          (ldst_expt),
        .exu_state_slv          (exu_state),
        .trap_exu_ctrl_mst      (trap_exu_ctrl),
        .csr_trap_state_slv     (csr_trap_state),
        .trap_csr_write_req_mst (trap_csr_write_req),
        .csr_trap_write_rsp_slv (csr_trap_write_rsp),
        .trap_send_mst          (trap_send)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        #100_000;
        $display("FAIL: trap_expt_priority_tb watchdog timeout");
        $finish;
    end

    task automatic tick(input int unsigned count = 1);
        repeat (count) @(posedge clk);
    endtask

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            saw_scause <= 1'b0;
            saw_sepc <= 1'b0;
            saw_stval <= 1'b0;
        end else if (trap_csr_write_req.vld) begin
            case (trap_csr_write_req.pkt.addr)
            12'h142: begin
                if (trap_csr_write_req.pkt.val !== 32'd13) begin
                    $display("FAIL: younger IFU exception won scause=%08x",
                        trap_csr_write_req.pkt.val);
                    $finish;
                end
                saw_scause <= 1'b1;
            end
            12'h141: begin
                if (trap_csr_write_req.pkt.val !== 32'h00010ffc) begin
                    $display("FAIL: wrong exception sepc=%08x",
                        trap_csr_write_req.pkt.val);
                    $finish;
                end
                saw_sepc <= 1'b1;
            end
            12'h143: begin
                if (trap_csr_write_req.pkt.val !== 32'h40bad004) begin
                    $display("FAIL: wrong exception stval=%08x",
                        trap_csr_write_req.pkt.val);
                    $finish;
                end
                saw_stval <= 1'b1;
            end
            default: begin end
            endcase
        end
    end

    initial begin
        rst_n = 1'b0;
        ex_expt.vld = 1'b0;
        ex_expt.pkt = '0;
        fch_expt.vld = 1'b0;
        fch_expt.pkt = '0;
        ldst_expt.vld = 1'b0;
        ldst_expt.pkt = '0;
        exu_state.pkt = '0;
        csr_trap_state.pkt = '0;
        csr_trap_write_rsp.vld = 1'b1;
        csr_trap_write_rsp.pkt.ok = 1'b1;

        csr_trap_state.pkt.medeleg = (32'b1 << 12) | (32'b1 << 13);
        csr_trap_state.pkt.stvec = 32'h40000200;
        exu_state.pkt.priv = 2'b00;

        tick(5);
        rst_n = 1'b1;
        tick(2);

        @(negedge clk);
        fch_expt.pkt.expt_type = HART_EXPT_TYPE_EXCEPTION;
        fch_expt.pkt.cause = HART_EXPT_CAUSE_INST_PAGE_FAULT;
        fch_expt.pkt.priv = 2'b00;
        fch_expt.pkt.pc = 32'h00011000;
        fch_expt.pkt.tval = 32'h00011000;
        fch_expt.vld = 1'b1;

        ldst_expt.pkt.expt_type = HART_EXPT_TYPE_EXCEPTION;
        ldst_expt.pkt.cause = HART_EXPT_CAUSE_LOAD_PAGE_FAULT;
        ldst_expt.pkt.priv = 2'b00;
        ldst_expt.pkt.pc = 32'h00010ffc;
        ldst_expt.pkt.tval = 32'h40bad004;
        ldst_expt.vld = 1'b1;
        tick();

        @(negedge clk);
        fch_expt.vld = 1'b0;
        ldst_expt.vld = 1'b0;

        do begin
            tick();
        end while (!trap_send.vld);

        if (!saw_scause || !saw_sepc || !saw_stval) begin
            $display("FAIL: incomplete trap writes cause=%0d epc=%0d tval=%0d",
                saw_scause, saw_sepc, saw_stval);
            $finish;
        end
        if (trap_send.pkt.target_pc !== 32'h40000200) begin
            $display("FAIL: wrong trap target=%08x", trap_send.pkt.target_pc);
            $finish;
        end

        $display("PASS: trap_expt_priority_tb");
        $finish;
    end
endmodule
