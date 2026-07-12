`include "spec/core/hart.svh"

module hart(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.mst  i_axi4_aw_mst,
    axi4_w_if_t.mst   i_axi4_w_mst,
    axi4_b_if_t.slv   i_axi4_b_slv,
    axi4_ar_if_t.mst  i_axi4_ar_mst,
    axi4_r_if_t.slv   i_axi4_r_slv,
    axi4_aw_if_t.mst  d_axi4_aw_mst,
    axi4_w_if_t.mst   d_axi4_w_mst,
    axi4_b_if_t.slv   d_axi4_b_slv,
    axi4_ar_if_t.mst  d_axi4_ar_mst,
    axi4_r_if_t.slv   d_axi4_r_slv,
    core_timer_if_t.slv core_timer_slv,
    core_m_irq_if_t.slv core_m_irq_slv,
    ext_irq_if_t.slv ext_irq_slv
);
    fch_req_if_t fch_req_if();
    fch_rsp_if_t fch_rsp_if();
    ex_req_if_t ex_req_if();
    ex_rsp_if_t ex_rsp_if();
    fl_req_if_t fl_req_if();
    ldst_req_if_t exu_lsu_ldst_req_if();
    ldst_rsp_if_t exu_lsu_ldst_rsp_if();
    ldst_req_if_t lsu_hbi_ldst_req_if();
    ldst_rsp_if_t lsu_hbi_ldst_rsp_if();
    exu_csr_read_req_if_t exu_csr_read_req_if();
    csr_exu_read_rsp_if_t csr_exu_read_rsp_if();
    exu_csr_write_req_if_t exu_csr_write_req_if();
    csr_exu_write_rsp_if_t csr_exu_write_rsp_if();
    hart_expt_if_t ex_expt_if();
    exu_state_if_t exu_state_if();
    trap_exu_ctrl_if_t trap_exu_ctrl_if();
    csr_trap_state_if_t csr_trap_state_if();
    trap_csr_write_req_if_t trap_csr_write_req_if();
    csr_trap_write_rsp_if_t csr_trap_write_rsp_if();
    trap_send_if_t trap_send_if();
    csr_mmu_state_if_t csr_mmu_state_if();
    csr_lsu_state_if_t csr_lsu_state_if();
    tlb_flush_if_t tlb_flush_if();
    l1_flush_if_t l1i_flush_if();
    l1_flush_if_t l1d_flush_if();
    hart_expt_if_t fch_expt_if();
    hart_expt_if_t ldst_expt_if();

    bti_req_if_t hbi_i_bti_req_if();
    bti_rsp_if_t hbi_i_bti_rsp_if();
    bti_req_if_t hbi_d_bti_req_if();
    bti_rsp_if_t hbi_d_bti_rsp_if();
    bti_req_if_t pa_i_bti_req_if();
    bti_rsp_if_t pa_i_bti_rsp_if();
    bti_req_if_t pa_d_bti_req_if();
    bti_rsp_if_t pa_d_bti_rsp_if();

    ifu u_ifu(
        .clk         (clk),
        .rst_n       (rst_n),
        .fch_req_mst (fch_req_if),
        .fch_rsp_slv (fch_rsp_if),
        .ex_req_mst  (ex_req_if),
        .ex_rsp_slv  (ex_rsp_if),
        .fl_req_mst  (fl_req_if),
        .tlb_flush_slv  (tlb_flush_if),
        .l1i_flush_vld  (l1i_flush_if.vld),
        .trap_send_slv (trap_send_if)
    );

    exu u_exu(
        .clk          (clk),
        .rst_n        (rst_n),
        .ex_req_slv   (ex_req_if),
        .ex_rsp_mst   (ex_rsp_if),
        .fl_req_slv   (fl_req_if),
        .ldst_req_mst (exu_lsu_ldst_req_if),
        .ldst_rsp_slv (exu_lsu_ldst_rsp_if),
        .exu_csr_read_req_mst  (exu_csr_read_req_if),
        .csr_exu_read_rsp_slv  (csr_exu_read_rsp_if),
        .exu_csr_write_req_mst (exu_csr_write_req_if),
        .csr_exu_write_rsp_slv (csr_exu_write_rsp_if),
        .tlb_flush_mst         (tlb_flush_if),
        .l1i_flush_mst         (l1i_flush_if),
        .l1d_flush_mst         (l1d_flush_if),
        .ex_expt_mst           (ex_expt_if),
        .exu_state_mst         (exu_state_if),
        .trap_exu_ctrl_slv     (trap_exu_ctrl_if)
    );

    csr u_csr(
        .clk                    (clk),
        .rst_n                  (rst_n),
        .exu_csr_read_req_slv   (exu_csr_read_req_if),
        .csr_exu_read_rsp_mst   (csr_exu_read_rsp_if),
        .exu_csr_write_req_slv  (exu_csr_write_req_if),
        .csr_exu_write_rsp_mst  (csr_exu_write_rsp_if),
        .core_timer_slv         (core_timer_slv),
        .core_m_irq_slv         (core_m_irq_slv),
        .ext_irq_slv            (ext_irq_slv),
        .trap_csr_write_req_slv (trap_csr_write_req_if),
        .csr_trap_write_rsp_mst (csr_trap_write_rsp_if),
        .csr_trap_state_mst     (csr_trap_state_if),
        .csr_mmu_state_mst      (csr_mmu_state_if),
        .csr_lsu_state_mst      (csr_lsu_state_if)
    );

    trap u_trap(
        .clk                    (clk),
        .rst_n                  (rst_n),
        .ex_expt_slv            (ex_expt_if),
        .fch_expt_slv           (fch_expt_if),
        .ldst_expt_slv          (ldst_expt_if),
        .exu_state_slv          (exu_state_if),
        .trap_exu_ctrl_mst      (trap_exu_ctrl_if),
        .csr_trap_state_slv     (csr_trap_state_if),
        .trap_csr_write_req_mst (trap_csr_write_req_if),
        .csr_trap_write_rsp_slv (csr_trap_write_rsp_if),
        .trap_send_mst          (trap_send_if)
    );

    lsu u_lsu(
        .clk               (clk),
        .rst_n             (rst_n),
        .exu_ldst_req_slv  (exu_lsu_ldst_req_if),
        .exu_ldst_rsp_mst  (exu_lsu_ldst_rsp_if),
        .hbi_ldst_req_mst  (lsu_hbi_ldst_req_if),
        .hbi_ldst_rsp_slv  (lsu_hbi_ldst_rsp_if),
        .csr_lsu_state_slv (csr_lsu_state_if)
    );

    mmu u_mmu(
        .clk               (clk),
        .rst_n             (rst_n),
        .va_i_req_slv      (hbi_i_bti_req_if),
        .va_i_rsp_mst      (hbi_i_bti_rsp_if),
        .va_d_req_slv      (hbi_d_bti_req_if),
        .va_d_rsp_mst      (hbi_d_bti_rsp_if),
        .pa_i_req_mst      (pa_i_bti_req_if),
        .pa_i_rsp_slv      (pa_i_bti_rsp_if),
        .pa_d_req_mst      (pa_d_bti_req_if),
        .pa_d_rsp_slv      (pa_d_bti_rsp_if),
        .exu_state_slv     (exu_state_if),
        .csr_mmu_state_slv (csr_mmu_state_if),
        .tlb_flush_slv     (tlb_flush_if),
        .fch_expt_mst      (fch_expt_if),
        .ldst_expt_mst     (ldst_expt_if)
    );

    hbi u_hbi(
        .clk           (clk),
        .rst_n         (rst_n),
        .fch_req_slv   (fch_req_if),
        .fch_rsp_mst   (fch_rsp_if),
        .ldst_req_slv  (lsu_hbi_ldst_req_if),
        .ldst_rsp_mst  (lsu_hbi_ldst_rsp_if),
        .i_bti_req_mst (hbi_i_bti_req_if),
        .i_bti_rsp_slv (hbi_i_bti_rsp_if),
        .d_bti_req_mst (hbi_d_bti_req_if),
        .d_bti_rsp_slv (hbi_d_bti_rsp_if)
    );

    l1 #(
        .RO           (1),
        .SIZE         (`L1I_SIZE),
        .WAY_NUM      (`L1I_WAY_NUM),
        .BYPASS0_BASE (32'h00000000),
        .BYPASS0_SIZE (32'h00000800),
        .BYPASS1_BASE (32'h10000000),
        .BYPASS1_SIZE (32'h00080000)
    ) u_l1i(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (pa_i_bti_req_if),
        .host_bti_rsp_mst (pa_i_bti_rsp_if),
        .flush_slv        (l1i_flush_if),
        .mem_axi4_aw_mst   (i_axi4_aw_mst),
        .mem_axi4_w_mst    (i_axi4_w_mst),
        .mem_axi4_b_slv    (i_axi4_b_slv),
        .mem_axi4_ar_mst   (i_axi4_ar_mst),
        .mem_axi4_r_slv    (i_axi4_r_slv)
    );

    l1 #(
        .RO           (0),
        .FULL_BYPASS  (0),
        .SIZE         (`L1D_SIZE),
        .WAY_NUM      (`L1D_WAY_NUM),
        .BYPASS0_BASE (32'h00000000),
        .BYPASS0_SIZE (32'h00000800),
        .BYPASS1_BASE (32'h10000000),
        .BYPASS1_SIZE (32'h00080000),
        .BYPASS2_BASE (32'h20000000),
        .BYPASS2_SIZE (32'h00040000),
        .BYPASS3_BASE (32'h30000000),
        .BYPASS3_SIZE (32'h02000000)
    ) u_l1d(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (pa_d_bti_req_if),
        .host_bti_rsp_mst (pa_d_bti_rsp_if),
        .flush_slv        (l1d_flush_if),
        .mem_axi4_aw_mst   (d_axi4_aw_mst),
        .mem_axi4_w_mst    (d_axi4_w_mst),
        .mem_axi4_b_slv    (d_axi4_b_slv),
        .mem_axi4_ar_mst   (d_axi4_ar_mst),
        .mem_axi4_r_slv    (d_axi4_r_slv)
    );

`ifndef SYNTHESIS
    logic rtl_progress_en;
    logic rtl_udelay_probe_en;
    longint unsigned rtl_progress_cycle;

    initial begin
        rtl_progress_en = $test$plusargs("rtl_progress");
        rtl_udelay_probe_en = $test$plusargs("rtl_udelay_probe");
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rtl_progress_cycle <= 0;
        end else if (rtl_progress_en) begin
            rtl_progress_cycle <= rtl_progress_cycle + 1;
            if (rtl_progress_cycle[19:0] == 20'h0) begin
                $display("[RTL_PROGRESS][%m] cycle=%0d pc=%08x ex=%0b/%0b fch=%0b/%0b:%0b/%0b ldst=%0b/%0b:%0b/%0b pa_i=%0b/%0b:%0b/%0b pa_d=%0b/%0b:%0b/%0b flush_i=%0b/%0b flush_d=%0b/%0b",
                    rtl_progress_cycle,
                    exu_state_if.pkt.pc,
                    ex_req_if.vld, ex_req_if.rdy,
                    fch_req_if.vld, fch_req_if.rdy,
                    fch_rsp_if.vld, fch_rsp_if.rdy,
                    lsu_hbi_ldst_req_if.vld, lsu_hbi_ldst_req_if.rdy,
                    lsu_hbi_ldst_rsp_if.vld, lsu_hbi_ldst_rsp_if.rdy,
                    pa_i_bti_req_if.vld, pa_i_bti_req_if.rdy,
                    pa_i_bti_rsp_if.vld, pa_i_bti_rsp_if.rdy,
                    pa_d_bti_req_if.vld, pa_d_bti_req_if.rdy,
                    pa_d_bti_rsp_if.vld, pa_d_bti_rsp_if.rdy,
                    l1i_flush_if.vld, l1i_flush_if.rdy,
                    l1d_flush_if.vld, l1d_flush_if.rdy);
            end
        end
    end

    always_ff @(posedge clk) begin
        if (rst_n && rtl_udelay_probe_en &&
            exu_state_if.pkt.pc >= 32'hc02de950 &&
            exu_state_if.pkt.pc < 32'hc02de980 &&
            rtl_progress_cycle[19:0] == 20'h0) begin
            $display("[RTL_UDELAY][%m] cycle=%0d pc=%08x ra=%08x s1=%08x mtime=%016x csr_time=%08x_%08x a3=%08x a4=%08x a5=%08x delta=%08x",
                rtl_progress_cycle,
                exu_state_if.pkt.pc,
                u_exu.u_exu_gpr.gprs[1],
                u_exu.u_exu_gpr.gprs[9],
                core_timer_slv.pkt.mtime,
                u_csr.csr_timeh,
                u_csr.csr_time,
                u_exu.u_exu_gpr.gprs[13],
                u_exu.u_exu_gpr.gprs[14],
                u_exu.u_exu_gpr.gprs[15],
                u_exu.u_exu_gpr.gprs[15] - u_exu.u_exu_gpr.gprs[13]);
        end
    end
`endif
endmodule
