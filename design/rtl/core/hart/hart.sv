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
    ldst_req_if_t ldst_req_if();
    ldst_rsp_if_t ldst_rsp_if();
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

    bti_req_if_t hbi_i_bti_req_if();
    bti_rsp_if_t hbi_i_bti_rsp_if();
    bti_req_if_t hbi_d_bti_req_if();
    bti_rsp_if_t hbi_d_bti_rsp_if();

    ifu u_ifu(
        .clk         (clk),
        .rst_n       (rst_n),
        .fch_req_mst (fch_req_if),
        .fch_rsp_slv (fch_rsp_if),
        .ex_req_mst  (ex_req_if),
        .ex_rsp_slv  (ex_rsp_if),
        .fl_req_mst  (fl_req_if),
        .trap_send_slv (trap_send_if)
    );

    exu u_exu(
        .clk          (clk),
        .rst_n        (rst_n),
        .ex_req_slv   (ex_req_if),
        .ex_rsp_mst   (ex_rsp_if),
        .fl_req_slv   (fl_req_if),
        .ldst_req_mst (ldst_req_if),
        .ldst_rsp_slv (ldst_rsp_if),
        .exu_csr_read_req_mst  (exu_csr_read_req_if),
        .csr_exu_read_rsp_slv  (csr_exu_read_rsp_if),
        .exu_csr_write_req_mst (exu_csr_write_req_if),
        .csr_exu_write_rsp_slv (csr_exu_write_rsp_if),
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
        .csr_trap_state_mst     (csr_trap_state_if)
    );

    trap u_trap(
        .clk                    (clk),
        .rst_n                  (rst_n),
        .ex_expt_slv            (ex_expt_if),
        .exu_state_slv          (exu_state_if),
        .trap_exu_ctrl_mst      (trap_exu_ctrl_if),
        .csr_trap_state_slv     (csr_trap_state_if),
        .trap_csr_write_req_mst (trap_csr_write_req_if),
        .csr_trap_write_rsp_slv (csr_trap_write_rsp_if),
        .trap_send_mst          (trap_send_if)
    );

    hbi u_hbi(
        .clk           (clk),
        .rst_n         (rst_n),
        .fch_req_slv   (fch_req_if),
        .fch_rsp_mst   (fch_rsp_if),
        .ldst_req_slv  (ldst_req_if),
        .ldst_rsp_mst  (ldst_rsp_if),
        .i_bti_req_mst (hbi_i_bti_req_if),
        .i_bti_rsp_slv (hbi_i_bti_rsp_if),
        .d_bti_req_mst (hbi_d_bti_req_if),
        .d_bti_rsp_slv (hbi_d_bti_rsp_if)
    );

    l1 u_l1i(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (hbi_i_bti_req_if),
        .host_bti_rsp_mst (hbi_i_bti_rsp_if),
        .mem_axi4_aw_mst   (i_axi4_aw_mst),
        .mem_axi4_w_mst    (i_axi4_w_mst),
        .mem_axi4_b_slv    (i_axi4_b_slv),
        .mem_axi4_ar_mst   (i_axi4_ar_mst),
        .mem_axi4_r_slv    (i_axi4_r_slv)
    );

    l1 u_l1d(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (hbi_d_bti_req_if),
        .host_bti_rsp_mst (hbi_d_bti_rsp_if),
        .mem_axi4_aw_mst   (d_axi4_aw_mst),
        .mem_axi4_w_mst    (d_axi4_w_mst),
        .mem_axi4_b_slv    (d_axi4_b_slv),
        .mem_axi4_ar_mst   (d_axi4_ar_mst),
        .mem_axi4_r_slv    (d_axi4_r_slv)
    );
endmodule
