`timescale 1ns/1ps

`include "itf/hart_expt_if.svh"

module ifu_fch_expt_defer_tb;
    logic clk;
    logic rst_n;
    logic saw_deferred_exception;

    fch_req_if_t fch_req();
    fch_rsp_if_t fch_rsp();
    ex_req_if_t ex_req();
    ex_rsp_if_t ex_rsp();
    fl_req_if_t fl_req();
    bpu_pred_req_if_t bpu_pred_req();
    bpu_pred_rsp_if_t bpu_pred_rsp();
    bpu_update_if_t bpu_update();
    tlb_flush_if_t tlb_flush();
    hart_expt_if_t fch_expt();
    trap_send_if_t trap_send();
    exu_state_if_t exu_state();
    perf_ifu_if_t perf();

    ifu u_dut(
        .clk              (clk),
        .rst_n            (rst_n),
        .fch_req_mst      (fch_req),
        .fch_rsp_slv      (fch_rsp),
        .ex_req_mst       (ex_req),
        .ex_rsp_slv       (ex_rsp),
        .fl_req_mst       (fl_req),
        .bpu_pred_req_mst (bpu_pred_req),
        .bpu_pred_rsp_slv (bpu_pred_rsp),
        .bpu_update_mst   (bpu_update),
        .tlb_flush_slv    (tlb_flush),
        .l1i_flush_vld    (1'b0),
        .fch_expt_mst     (fch_expt),
        .trap_send_slv    (trap_send),
        .exu_state_slv    (exu_state),
        .perf_mst         (perf)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        #100_000;
        $display("FAIL: ifu_fch_expt_defer_tb watchdog timeout");
        $finish;
    end

    assign fch_req.rdy = 1'b1;
    assign ex_req.rdy = 1'b0;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            saw_deferred_exception <= 1'b0;
        else if (fch_expt.vld && exu_state.pkt.trap_defer)
            saw_deferred_exception <= 1'b1;
    end

    task automatic tick(input int unsigned count = 1);
        repeat (count) @(posedge clk);
    endtask

    initial begin
        rst_n = 1'b0;
        fch_rsp.vld = 1'b0;
        fch_rsp.pkt = '0;
        ex_rsp.vld = 1'b0;
        ex_rsp.pkt = '0;
        bpu_pred_rsp.pkt = '0;
        tlb_flush.vld = 1'b0;
        trap_send.vld = 1'b0;
        trap_send.pkt = '0;
        exu_state.pkt = '0;
        exu_state.pkt.priv = 2'b00;
        exu_state.pkt.trap_defer = 1'b1;

        tick(5);
        rst_n = 1'b1;
        do begin
            tick();
        end while (!(fch_req.vld && fch_req.rdy));

        @(negedge clk);
        fch_rsp.pkt.ir = 32'b0;
        fch_rsp.pkt.ok = 1'b0;
        fch_rsp.pkt.expt = 1'b1;
        fch_rsp.pkt.cause = HART_EXPT_CAUSE_INST_PAGE_FAULT;
        fch_rsp.pkt.priv = 2'b00;
        fch_rsp.pkt.tval = 32'h00011000;
        fch_rsp.vld = 1'b1;
        do begin
            tick();
        end while (!fch_rsp.rdy);
        @(negedge clk);
        fch_rsp.vld = 1'b0;

        tick(3);
        if (saw_deferred_exception) begin
            $display("FAIL: IFU released younger fetch exception while EXU busy");
            $finish;
        end

        exu_state.pkt.trap_defer = 1'b0;
        do begin
            tick();
        end while (!fch_expt.vld);
        if (fch_expt.pkt.pc !== 32'h00000000 ||
            fch_expt.pkt.cause != HART_EXPT_CAUSE_INST_PAGE_FAULT) begin
            $display("FAIL: deferred exception payload pc=%08x cause=%0d",
                fch_expt.pkt.pc, fch_expt.pkt.cause);
            $finish;
        end

        $display("PASS: ifu_fch_expt_defer_tb");
        $finish;
    end
endmodule
