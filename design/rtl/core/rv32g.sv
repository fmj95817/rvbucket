`include "spec/core/isa.svh"
`include "boot.svh"

module rv32g(
    input logic       clk,
    input logic       rst_n,
    apb_req_if_t.mst  peri_apb_req_mst,
    apb_rsp_if_t.slv  peri_apb_rsp_slv,
    axi4_aw_if_t.mst  mm_axi4_aw_mst,
    axi4_w_if_t.mst   mm_axi4_w_mst,
    axi4_b_if_t.slv   mm_axi4_b_slv,
    axi4_ar_if_t.mst  mm_axi4_ar_mst,
    axi4_r_if_t.slv   mm_axi4_r_slv,
    perf_rv32g_if_t.mst perf_mst,
    perf_axi_demux_if_t.mst perf_cbi_i_axi_demux_mst,
    perf_axi_demux_if_t.mst perf_cbi_d_axi_demux_mst,
    perf_l2_if_t.mst        perf_l2_mst,
    perf_ifu_if_t.mst       perf_ifu_mst,
    perf_bpu_if_t.mst       perf_bpu_mst,
    perf_exu_if_t.mst       perf_exu_mst,
    perf_lsu_if_t.mst       perf_lsu_mst,
    perf_mmu_if_t.mst       perf_mmu_mst,
    perf_l1_if_t.mst        perf_l1i_mst,
    perf_l1_if_t.mst        perf_l1d_mst,
    ext_irq_if_t.slv  uart_irq_slv,
    ext_irq_if_t.slv  gpio_irq_slv,
    ext_irq_if_t.slv  gtimer_irq_slv
);
    localparam BOOT_ROM_AW = `BOOT_ROM_WORD_AW + 2;

    axi4_aw_if_t hart_i_aw();
    axi4_w_if_t hart_i_w();
    axi4_b_if_t hart_i_b();
    axi4_ar_if_t hart_i_ar();
    axi4_r_if_t hart_i_r();
    axi4_aw_if_t cbi_i_aw();
    axi4_w_if_t cbi_i_w();
    axi4_b_if_t cbi_i_b();
    axi4_ar_if_t cbi_i_ar();
    axi4_r_if_t cbi_i_r();
    axi4_aw_if_t hart_d_aw();
    axi4_w_if_t hart_d_w();
    axi4_b_if_t hart_d_b();
    axi4_ar_if_t hart_d_ar();
    axi4_r_if_t hart_d_r();
    axi4_aw_if_t cbi_d_aw();
    axi4_w_if_t cbi_d_w();
    axi4_b_if_t cbi_d_b();
    axi4_ar_if_t cbi_d_ar();
    axi4_r_if_t cbi_d_r();
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
    apb_req_if_t cfg_req();
    apb_rsp_if_t cfg_rsp();
    apb_req_if_t cfg_gst_req[3]();
    apb_rsp_if_t cfg_gst_rsp[3]();
    apb_req_if_t aclint_req();
    apb_rsp_if_t aclint_rsp();
    apb_req_if_t plic_req();
    apb_rsp_if_t plic_rsp();
    bti_req_if_t boot_rom_req();
    bti_rsp_if_t boot_rom_rsp();
    core_timer_if_t core_timer();
    core_m_irq_if_t core_m_irq();
    ext_irq_if_t ext_irq();
    logic boot_rom_cs;
    logic [`BOOT_ROM_WORD_AW-1:0] boot_rom_addr;
    logic [`RV_XLEN-1:0] boot_rom_data;
    hart u_hart(
        .clk            (clk),
        .rst_n          (rst_n),
        .i_axi4_aw_mst  (hart_i_aw),
        .i_axi4_w_mst   (hart_i_w),
        .i_axi4_b_slv   (hart_i_b),
        .i_axi4_ar_mst  (hart_i_ar),
        .i_axi4_r_slv   (hart_i_r),
        .d_axi4_aw_mst  (hart_d_aw),
        .d_axi4_w_mst   (hart_d_w),
        .d_axi4_b_slv   (hart_d_b),
        .d_axi4_ar_mst  (hart_d_ar),
        .d_axi4_r_slv   (hart_d_r),
        .core_timer_slv (core_timer),
        .core_m_irq_slv (core_m_irq),
        .ext_irq_slv    (ext_irq),
        .perf_ifu_mst   (perf_ifu_mst),
        .perf_bpu_mst   (perf_bpu_mst),
        .perf_exu_mst   (perf_exu_mst),
        .perf_lsu_mst   (perf_lsu_mst),
        .perf_mmu_mst   (perf_mmu_mst),
        .perf_l1i_mst   (perf_l1i_mst),
        .perf_l1d_mst   (perf_l1d_mst)
    );

    axi_link u_hart_i_axi_link(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (hart_i_aw),
        .host_axi4_w_slv  (hart_i_w),
        .host_axi4_b_mst  (hart_i_b),
        .host_axi4_ar_slv (hart_i_ar),
        .host_axi4_r_mst  (hart_i_r),
        .gst_axi4_aw_mst  (cbi_i_aw),
        .gst_axi4_w_mst   (cbi_i_w),
        .gst_axi4_b_slv   (cbi_i_b),
        .gst_axi4_ar_mst  (cbi_i_ar),
        .gst_axi4_r_slv   (cbi_i_r)
    );

    axi_link u_hart_d_axi_link(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (hart_d_aw),
        .host_axi4_w_slv  (hart_d_w),
        .host_axi4_b_mst  (hart_d_b),
        .host_axi4_ar_slv (hart_d_ar),
        .host_axi4_r_mst  (hart_d_r),
        .gst_axi4_aw_mst  (cbi_d_aw),
        .gst_axi4_w_mst   (cbi_d_w),
        .gst_axi4_b_slv   (cbi_d_b),
        .gst_axi4_ar_mst  (cbi_d_ar),
        .gst_axi4_r_slv   (cbi_d_r)
    );

    cbi u_cbi(
        .clk                  (clk),
        .rst_n                (rst_n),
        .hart_i_axi4_aw_slv   (cbi_i_aw),
        .hart_i_axi4_w_slv    (cbi_i_w),
        .hart_i_axi4_b_mst    (cbi_i_b),
        .hart_i_axi4_ar_slv   (cbi_i_ar),
        .hart_i_axi4_r_mst    (cbi_i_r),
        .hart_d_axi4_aw_slv   (cbi_d_aw),
        .hart_d_axi4_w_slv    (cbi_d_w),
        .hart_d_axi4_b_mst    (cbi_d_b),
        .hart_d_axi4_ar_slv   (cbi_d_ar),
        .hart_d_axi4_r_mst    (cbi_d_r),
        .boot_rom_bti_req_mst (boot_rom_req),
        .boot_rom_bti_rsp_slv (boot_rom_rsp),
        .cfg_apb_req_mst      (cfg_req),
        .cfg_apb_rsp_slv      (cfg_rsp),
        .mm_i_axi4_aw_mst     (mm_i_aw),
        .mm_i_axi4_w_mst      (mm_i_w),
        .mm_i_axi4_b_slv      (mm_i_b),
        .mm_i_axi4_ar_mst     (mm_i_ar),
        .mm_i_axi4_r_slv      (mm_i_r),
        .mm_d_axi4_aw_mst     (mm_d_aw),
        .mm_d_axi4_w_mst      (mm_d_w),
        .mm_d_axi4_b_slv      (mm_d_b),
        .mm_d_axi4_ar_mst     (mm_d_ar),
        .mm_d_axi4_r_slv      (mm_d_r),
        .perf_i_axi_demux_mst (perf_cbi_i_axi_demux_mst),
        .perf_d_axi_demux_mst (perf_cbi_d_axi_demux_mst)
    );

    apb_demux #(
        .GST_NUM  (3),
`ifdef VERILATOR
        .GST_BASE ('{32'h30000000, 32'h31000000, 32'h31100000,
            32'h00000000}),
        .GST_SIZE ('{32'h01000000, 32'h00100000, 32'h00400000,
            32'h00000000})
`else
        .GST_BASE ('{32'h30000000, 32'h31000000, 32'h31100000}),
        .GST_SIZE ('{32'h01000000, 32'h00100000, 32'h00400000})
`endif
    ) u_cfg_apb_demux(
        .host_apb_req_slv  (cfg_req),
        .host_apb_rsp_mst  (cfg_rsp),
        .gst_apb_req_msts  (cfg_gst_req),
        .gst_apb_rsp_slvs  (cfg_gst_rsp)
    );

    assign peri_apb_req_mst.psel = cfg_gst_req[0].psel;
    assign peri_apb_req_mst.penable = cfg_gst_req[0].penable;
    assign peri_apb_req_mst.pkt = cfg_gst_req[0].pkt;
    assign cfg_gst_rsp[0].pready = peri_apb_rsp_slv.pready;
    assign cfg_gst_rsp[0].pkt = peri_apb_rsp_slv.pkt;

    assign aclint_req.psel = cfg_gst_req[1].psel;
    assign aclint_req.penable = cfg_gst_req[1].penable;
    assign aclint_req.pkt = cfg_gst_req[1].pkt;
    assign cfg_gst_rsp[1].pready = aclint_rsp.pready;
    assign cfg_gst_rsp[1].pkt = aclint_rsp.pkt;

    assign plic_req.psel = cfg_gst_req[2].psel;
    assign plic_req.penable = cfg_gst_req[2].penable;
    assign plic_req.pkt = cfg_gst_req[2].pkt;
    assign cfg_gst_rsp[2].pready = plic_rsp.pready;
    assign cfg_gst_rsp[2].pkt = plic_rsp.pkt;

    aclint u_aclint(
        .clk              (clk),
        .rst_n            (rst_n),
        .apb_req_slv      (aclint_req),
        .apb_rsp_mst      (aclint_rsp),
        .core_timer_mst   (core_timer),
        .core_m_irq_mst   (core_m_irq)
    );

    plic u_plic(
        .clk             (clk),
        .rst_n           (rst_n),
        .apb_req_slv     (plic_req),
        .apb_rsp_mst     (plic_rsp),
        .uart_irq_slv    (uart_irq_slv),
        .gpio_irq_slv    (gpio_irq_slv),
        .gtimer_irq_slv  (gtimer_irq_slv),
        .core_irq_mst    (ext_irq)
    );

    boot_rom u_boot_rom(
        .clk  (clk),
        .cs   (boot_rom_cs),
        .addr (boot_rom_addr),
        .data (boot_rom_data)
    );

    bti_to_rom #(
        .BTI_AW (`RV_AW),
        .BTI_DW (`RV_XLEN),
        .ROM_AW (BOOT_ROM_AW)
    ) u_bti_to_boot_rom(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (boot_rom_req),
        .bti_rsp_mst (boot_rom_rsp),
        .cs          (boot_rom_cs),
        .addr        (boot_rom_addr),
        .data        (boot_rom_data)
    );

    l2 u_l2(
        .clk              (clk),
        .rst_n            (rst_n),
        .i_axi4_aw_slv    (mm_i_aw),
        .i_axi4_w_slv     (mm_i_w),
        .i_axi4_b_mst     (mm_i_b),
        .i_axi4_ar_slv    (mm_i_ar),
        .i_axi4_r_mst     (mm_i_r),
        .d_axi4_aw_slv    (mm_d_aw),
        .d_axi4_w_slv     (mm_d_w),
        .d_axi4_b_mst     (mm_d_b),
        .d_axi4_ar_slv    (mm_d_ar),
        .d_axi4_r_mst     (mm_d_r),
        .mem_axi4_aw_mst  (mm_axi4_aw_mst),
        .mem_axi4_w_mst   (mm_axi4_w_mst),
        .mem_axi4_b_slv   (mm_axi4_b_slv),
        .mem_axi4_ar_mst  (mm_axi4_ar_mst),
        .mem_axi4_r_slv   (mm_axi4_r_slv),
        .perf_mst         (perf_l2_mst)
    );

    assign perf_mst.pkt.hart_i_aw_bp = hart_i_aw.vld && !hart_i_aw.rdy;
    assign perf_mst.pkt.hart_i_w_bp = hart_i_w.vld && !hart_i_w.rdy;
    assign perf_mst.pkt.hart_i_ar_bp = hart_i_ar.vld && !hart_i_ar.rdy;
    assign perf_mst.pkt.hart_i_b_bp = hart_i_b.vld && !hart_i_b.rdy;
    assign perf_mst.pkt.hart_i_r_bp = hart_i_r.vld && !hart_i_r.rdy;

    assign perf_mst.pkt.hart_d_aw_bp = hart_d_aw.vld && !hart_d_aw.rdy;
    assign perf_mst.pkt.hart_d_w_bp = hart_d_w.vld && !hart_d_w.rdy;
    assign perf_mst.pkt.hart_d_ar_bp = hart_d_ar.vld && !hart_d_ar.rdy;
    assign perf_mst.pkt.hart_d_b_bp = hart_d_b.vld && !hart_d_b.rdy;
    assign perf_mst.pkt.hart_d_r_bp = hart_d_r.vld && !hart_d_r.rdy;

    assign perf_mst.pkt.cbi_i_aw_bp = cbi_i_aw.vld && !cbi_i_aw.rdy;
    assign perf_mst.pkt.cbi_i_w_bp = cbi_i_w.vld && !cbi_i_w.rdy;
    assign perf_mst.pkt.cbi_i_ar_bp = cbi_i_ar.vld && !cbi_i_ar.rdy;
    assign perf_mst.pkt.cbi_i_b_bp = cbi_i_b.vld && !cbi_i_b.rdy;
    assign perf_mst.pkt.cbi_i_r_bp = cbi_i_r.vld && !cbi_i_r.rdy;

    assign perf_mst.pkt.cbi_d_aw_bp = cbi_d_aw.vld && !cbi_d_aw.rdy;
    assign perf_mst.pkt.cbi_d_w_bp = cbi_d_w.vld && !cbi_d_w.rdy;
    assign perf_mst.pkt.cbi_d_ar_bp = cbi_d_ar.vld && !cbi_d_ar.rdy;
    assign perf_mst.pkt.cbi_d_b_bp = cbi_d_b.vld && !cbi_d_b.rdy;
    assign perf_mst.pkt.cbi_d_r_bp = cbi_d_r.vld && !cbi_d_r.rdy;

    assign perf_mst.pkt.l2_i_aw_bp = mm_i_aw.vld && !mm_i_aw.rdy;
    assign perf_mst.pkt.l2_i_w_bp = mm_i_w.vld && !mm_i_w.rdy;
    assign perf_mst.pkt.l2_i_ar_bp = mm_i_ar.vld && !mm_i_ar.rdy;
    assign perf_mst.pkt.l2_i_b_bp = mm_i_b.vld && !mm_i_b.rdy;
    assign perf_mst.pkt.l2_i_r_bp = mm_i_r.vld && !mm_i_r.rdy;

    assign perf_mst.pkt.l2_d_aw_bp = mm_d_aw.vld && !mm_d_aw.rdy;
    assign perf_mst.pkt.l2_d_w_bp = mm_d_w.vld && !mm_d_w.rdy;
    assign perf_mst.pkt.l2_d_ar_bp = mm_d_ar.vld && !mm_d_ar.rdy;
    assign perf_mst.pkt.l2_d_b_bp = mm_d_b.vld && !mm_d_b.rdy;
    assign perf_mst.pkt.l2_d_r_bp = mm_d_r.vld && !mm_d_r.rdy;

    assign perf_mst.pkt.cfg_apb_bp =
        cfg_req.psel && cfg_req.penable && !cfg_rsp.pready;
endmodule
