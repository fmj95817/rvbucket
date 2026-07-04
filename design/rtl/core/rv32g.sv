module rv32g(
    input logic       clk,
    input logic       rst_n,
    bti_req_if_t.mst  boot_rom_bti_req_mst,
    bti_rsp_if_t.slv  boot_rom_bti_rsp_slv,
    bti_req_if_t.mst  itcm_i_bti_req_mst,
    bti_rsp_if_t.slv  itcm_i_bti_rsp_slv,
    bti_req_if_t.mst  itcm_d_bti_req_mst,
    bti_rsp_if_t.slv  itcm_d_bti_rsp_slv,
    bti_req_if_t.mst  dtcm_bti_req_mst,
    bti_rsp_if_t.slv  dtcm_bti_rsp_slv,
    bti_req_if_t.mst  cfg_bti_req_mst,
    bti_rsp_if_t.slv  cfg_bti_rsp_slv,
    axi4_aw_if_t.mst  mm_axi4_aw_mst,
    axi4_w_if_t.mst   mm_axi4_w_mst,
    axi4_b_if_t.slv   mm_axi4_b_slv,
    axi4_ar_if_t.mst  mm_axi4_ar_mst,
    axi4_r_if_t.slv   mm_axi4_r_slv
);
    axi4_aw_if_t hart_i_aw();
    axi4_w_if_t hart_i_w();
    axi4_b_if_t hart_i_b();
    axi4_ar_if_t hart_i_ar();
    axi4_r_if_t hart_i_r();
    axi4_aw_if_t hart_d_aw();
    axi4_w_if_t hart_d_w();
    axi4_b_if_t hart_d_b();
    axi4_ar_if_t hart_d_ar();
    axi4_r_if_t hart_d_r();
    axi4_aw_if_t mm_i_aw();
    axi4_w_if_t mm_i_w();
    axi4_b_if_t mm_i_b();
    axi4_ar_if_t mm_i_ar();
    axi4_r_if_t mm_i_r();
    axi4_aw_if_t mm_d_aw();
    axi4_w_if_t mm_d_w();
    axi4_b_if_t mm_d_b();
    axi4_ar_if_t mm_d_ar();
    axi4_r_if_t mm_d_r();
    bti_req_if_t cfg_req();
    bti_rsp_if_t cfg_rsp();
    bti_req_if_t aclint_req();
    bti_rsp_if_t aclint_rsp();
    bti_req_if_t plic_req();
    bti_rsp_if_t plic_rsp();
    core_timer_if_t core_timer();
    core_m_irq_if_t core_m_irq();
    ext_irq_if_t ext_irq();

    hart u_hart(
        clk, rst_n,
        hart_i_aw, hart_i_w, hart_i_b, hart_i_ar, hart_i_r,
        hart_d_aw, hart_d_w, hart_d_b, hart_d_ar, hart_d_r,
        core_timer, core_m_irq, ext_irq
    );

    cbi u_cbi(
        clk, rst_n,
        hart_i_aw, hart_i_w, hart_i_b, hart_i_ar, hart_i_r,
        hart_d_aw, hart_d_w, hart_d_b, hart_d_ar, hart_d_r,
        boot_rom_bti_req_mst, boot_rom_bti_rsp_slv,
        itcm_i_bti_req_mst, itcm_i_bti_rsp_slv,
        itcm_d_bti_req_mst, itcm_d_bti_rsp_slv,
        dtcm_bti_req_mst, dtcm_bti_rsp_slv,
        cfg_req, cfg_rsp,
        mm_i_aw, mm_i_w, mm_i_b, mm_i_ar, mm_i_r,
        mm_d_aw, mm_d_w, mm_d_b, mm_d_ar, mm_d_r
    );

    cfg_router u_cfg_router(
        clk, rst_n, cfg_req, cfg_rsp,
        cfg_bti_req_mst, cfg_bti_rsp_slv,
        aclint_req, aclint_rsp, plic_req, plic_rsp
    );

    aclint u_aclint(clk, rst_n, aclint_req, aclint_rsp, core_timer, core_m_irq);
    gpio u_plic_stub(clk, rst_n, plic_req, plic_rsp);
    assign ext_irq.pkt.irq = 1'b0;

    l2 u_l2(
        clk, rst_n,
        mm_i_aw, mm_i_w, mm_i_b, mm_i_ar, mm_i_r,
        mm_d_aw, mm_d_w, mm_d_b, mm_d_ar, mm_d_r,
        mm_axi4_aw_mst, mm_axi4_w_mst, mm_axi4_b_slv,
        mm_axi4_ar_mst, mm_axi4_r_slv
    );
endmodule
