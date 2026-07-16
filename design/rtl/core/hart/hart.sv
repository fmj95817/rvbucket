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
    ext_irq_if_t.slv ext_irq_slv,
    perf_ifu_if_t.mst perf_ifu_mst,
    perf_bpu_if_t.mst perf_bpu_mst,
    perf_exu_if_t.mst perf_exu_mst,
    perf_lsu_if_t.mst perf_lsu_mst,
    perf_mmu_if_t.mst perf_mmu_mst,
    perf_l1_if_t.mst  perf_l1i_mst,
    perf_l1_if_t.mst  perf_l1d_mst
);
    fch_req_if_t ifu_fch_req_if();
    fch_req_if_t hbi_fch_req_if();
    fch_rsp_if_t fch_rsp_if();
    ex_req_if_t ifu_ex_req_if();
    ex_req_if_t exu_ex_req_if();
    ex_rsp_if_t ex_rsp_if();
    fl_req_if_t fl_req_if();
    bpu_pred_req_if_t bpu_pred_req_if();
    bpu_pred_rsp_if_t bpu_pred_rsp_if();
    bpu_update_if_t bpu_update_if();
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
    l1_flush_ack_if_t l1i_flush_ack_if();
    l1_flush_ack_if_t l1d_flush_ack_if();
    hart_expt_if_t mmu_fch_expt_if();
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
    bti_req_if_t pa_ptw_bti_req_if();
    bti_rsp_if_t pa_ptw_bti_rsp_if();
    bti_req_if_t l1d_bti_req_if();
    bti_rsp_if_t l1d_bti_rsp_if();

    localparam int FCH_REQ_DW = 32;
    localparam int EX_REQ_DW = 98;
    logic [FCH_REQ_DW-1:0] fch_req_data;
    logic [EX_REQ_DW-1:0] ex_req_data;

    bidir_reg_slice #(
        .DW (FCH_REQ_DW)
    ) u_fch_req_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (ifu_fch_req_if.vld),
        .src_rdy  (ifu_fch_req_if.rdy),
        .src_data (ifu_fch_req_if.pkt),
        .dst_vld  (hbi_fch_req_if.vld),
        .dst_rdy  (hbi_fch_req_if.rdy),
        .dst_data (fch_req_data)
    );

    assign hbi_fch_req_if.pkt = fch_req_data;

    bidir_reg_slice #(
        .DW (EX_REQ_DW)
    ) u_ex_req_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (fl_req_if.vld || tlb_flush_if.vld || l1i_flush_if.vld),
        .src_vld  (ifu_ex_req_if.vld),
        .src_rdy  (ifu_ex_req_if.rdy),
        .src_data (ifu_ex_req_if.pkt),
        .dst_vld  (exu_ex_req_if.vld),
        .dst_rdy  (exu_ex_req_if.rdy),
        .dst_data (ex_req_data)
    );

    assign exu_ex_req_if.pkt = ex_req_data;

    ifu u_ifu(
        .clk         (clk),
        .rst_n       (rst_n),
        .fch_req_mst (ifu_fch_req_if),
        .fch_rsp_slv (fch_rsp_if),
        .ex_req_mst  (ifu_ex_req_if),
        .ex_rsp_slv  (ex_rsp_if),
        .fl_req_mst  (fl_req_if),
        .bpu_pred_req_mst (bpu_pred_req_if),
        .bpu_pred_rsp_slv (bpu_pred_rsp_if),
        .bpu_update_mst   (bpu_update_if),
        .tlb_flush_slv  (tlb_flush_if),
        .l1i_flush_vld  (l1i_flush_if.vld),
        .fch_expt_mst   (fch_expt_if),
        .trap_send_slv (trap_send_if),
        .exu_state_slv (exu_state_if),
        .perf_mst      (perf_ifu_mst)
    );

    bpu u_bpu(
        .clk          (clk),
        .rst_n        (rst_n),
        .pred_req_slv (bpu_pred_req_if),
        .pred_rsp_mst (bpu_pred_rsp_if),
        .update_slv   (bpu_update_if),
        .perf_mst     (perf_bpu_mst)
    );

    exu u_exu(
        .clk          (clk),
        .rst_n        (rst_n),
        .ex_req_slv   (exu_ex_req_if),
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
        .l1i_flush_ack_slv     (l1i_flush_ack_if),
        .l1d_flush_ack_slv     (l1d_flush_ack_if),
        .ex_expt_mst           (ex_expt_if),
        .exu_state_mst         (exu_state_if),
        .trap_exu_ctrl_slv     (trap_exu_ctrl_if),
        .perf_mst              (perf_exu_mst)
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
        .csr_lsu_state_slv (csr_lsu_state_if),
        .perf_mst          (perf_lsu_mst)
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
        .ptw_req_mst       (pa_ptw_bti_req_if),
        .ptw_rsp_slv       (pa_ptw_bti_rsp_if),
        .exu_state_slv     (exu_state_if),
        .csr_mmu_state_slv (csr_mmu_state_if),
        .tlb_flush_slv     (tlb_flush_if),
        .fch_expt_mst      (mmu_fch_expt_if),
        .ldst_expt_mst     (ldst_expt_if),
        .perf_mst          (perf_mmu_mst)
    );

    hbi u_hbi(
        .clk           (clk),
        .rst_n         (rst_n),
        .fch_req_slv   (hbi_fch_req_if),
        .fch_rsp_mst   (fch_rsp_if),
        .mmu_fch_expt_slv (mmu_fch_expt_if),
        .ldst_req_slv  (lsu_hbi_ldst_req_if),
        .ldst_rsp_mst  (lsu_hbi_ldst_rsp_if),
        .i_bti_req_mst (hbi_i_bti_req_if),
        .i_bti_rsp_slv (hbi_i_bti_rsp_if),
        .d_bti_req_mst (hbi_d_bti_req_if),
        .d_bti_rsp_slv (hbi_d_bti_rsp_if)
    );

    bti_mux2 #(
        .STG_FIFO_DEPTH (`HART_L1D_BTI_MUX_STG_FIFO_DEPTH),
        .OST_DEPTH      (`HART_L1D_BTI_MUX_OST_DEPTH)
    ) u_l1d_bti_mux(
        .clk           (clk),
        .rst_n         (rst_n),
        .host0_req_slv (pa_d_bti_req_if),
        .host0_rsp_mst (pa_d_bti_rsp_if),
        .host1_req_slv (pa_ptw_bti_req_if),
        .host1_rsp_mst (pa_ptw_bti_rsp_if),
        .gst_req_mst   (l1d_bti_req_if),
        .gst_rsp_slv   (l1d_bti_rsp_if)
    );

    l1 #(
        .RO             (1),
        .SIZE           (`L1I_SIZE),
        .WAY_NUM        (`L1I_WAY_NUM),
        .STG_FIFO_DEPTH (`HART_L1I_STG_FIFO_DEPTH),
        .OST_DEPTH      (`HART_L1_OST_DEPTH),
        .BYPASS0_BASE (32'h00000000),
        .BYPASS0_SIZE (32'h00000800)
    ) u_l1i(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (pa_i_bti_req_if),
        .host_bti_rsp_mst (pa_i_bti_rsp_if),
        .flush_slv        (l1i_flush_if),
        .flush_ack_mst    (l1i_flush_ack_if),
        .mem_axi4_aw_mst   (i_axi4_aw_mst),
        .mem_axi4_w_mst    (i_axi4_w_mst),
        .mem_axi4_b_slv    (i_axi4_b_slv),
        .mem_axi4_ar_mst   (i_axi4_ar_mst),
        .mem_axi4_r_slv    (i_axi4_r_slv),
        .perf_mst          (perf_l1i_mst)
    );

    l1 #(
        .RO             (0),
        .FULL_BYPASS    (0),
        .SIZE           (`L1D_SIZE),
        .WAY_NUM        (`L1D_WAY_NUM),
        .STG_FIFO_DEPTH (`HART_L1D_STG_FIFO_DEPTH),
        .OST_DEPTH      (`HART_L1_OST_DEPTH),
        .BYPASS0_BASE (32'h00000000),
        .BYPASS0_SIZE (32'h00000800),
        .BYPASS1_BASE (32'h30000000),
        .BYPASS1_SIZE (32'h02000000)
    ) u_l1d(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (l1d_bti_req_if),
        .host_bti_rsp_mst (l1d_bti_rsp_if),
        .flush_slv        (l1d_flush_if),
        .flush_ack_mst    (l1d_flush_ack_if),
        .mem_axi4_aw_mst   (d_axi4_aw_mst),
        .mem_axi4_w_mst    (d_axi4_w_mst),
        .mem_axi4_b_slv    (d_axi4_b_slv),
        .mem_axi4_ar_mst   (d_axi4_ar_mst),
        .mem_axi4_r_slv    (d_axi4_r_slv),
        .perf_mst          (perf_l1d_mst)
    );

endmodule
