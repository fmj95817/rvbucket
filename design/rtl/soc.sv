`include "spec/core/isa.svh"
`include "spec/soc.svh"
`include "dbg/pcm_perf.svh"

module soc(
    input logic         clk,
    input logic         rst_n,
    output logic        uart_tx,
    input logic         uart_rx,
    input logic [23:0]  gpio_in,
    output logic [23:0] gpio_out,
    output logic [23:0] gpio_oe,
    axi4_aw_if_t.mst   ddr_axi4_aw_mst,
    axi4_w_if_t.mst    ddr_axi4_w_mst,
    axi4_b_if_t.slv    ddr_axi4_b_slv,
    axi4_ar_if_t.mst   ddr_axi4_ar_mst,
    axi4_r_if_t.slv    ddr_axi4_r_slv,
    axi4_aw_if_t.mst   flash_axi4_aw_mst,
    axi4_w_if_t.mst    flash_axi4_w_mst,
    axi4_b_if_t.slv    flash_axi4_b_slv,
    axi4_ar_if_t.mst   flash_axi4_ar_mst,
    axi4_r_if_t.slv    flash_axi4_r_slv
);
    apb_req_if_t peri_req();
    apb_rsp_if_t peri_rsp();
    axi4_aw_if_t mm_aw();
    axi4_w_if_t mm_w();
    axi4_b_if_t mm_b();
    axi4_ar_if_t mm_ar();
    axi4_r_if_t mm_r();
    axi4_aw_if_t mm_gst_aw[2]();
    axi4_w_if_t mm_gst_w[2]();
    axi4_b_if_t mm_gst_b[2]();
    axi4_ar_if_t mm_gst_ar[2]();
    axi4_r_if_t mm_gst_r[2]();
    ext_irq_if_t uart_irq();
    ext_irq_if_t gpio_irq();
    ext_irq_if_t gtimer_irq();
    logic [`RVB_PCM_COUNTER_NUM-1:0] pcm_inc_en;
    perf_soc_if_t perf_soc_if();
    perf_rv32g_if_t perf_rv32g_if();
    perf_axi_demux_if_t perf_mm_axi_demux_if();
    perf_axi_demux_if_t perf_cbi_i_axi_demux_if();
    perf_axi_demux_if_t perf_cbi_d_axi_demux_if();
    perf_l2_if_t perf_l2_if();
    perf_ifu_if_t perf_ifu_if();
    perf_bpu_if_t perf_bpu_if();
    perf_exu_if_t perf_exu_if();
    perf_lsu_if_t perf_lsu_if();
    perf_mmu_if_t perf_mmu_if();
    perf_l1_if_t perf_l1i_if();
    perf_l1_if_t perf_l1d_if();

    rv32g u_rv32g(
        .clk              (clk),
        .rst_n            (rst_n),
        .peri_apb_req_mst (peri_req),
        .peri_apb_rsp_slv (peri_rsp),
        .mm_axi4_aw_mst   (mm_aw),
        .mm_axi4_w_mst    (mm_w),
        .mm_axi4_b_slv    (mm_b),
        .mm_axi4_ar_mst   (mm_ar),
        .mm_axi4_r_slv    (mm_r),
        .perf_mst                 (perf_rv32g_if),
        .perf_cbi_i_axi_demux_mst (perf_cbi_i_axi_demux_if),
        .perf_cbi_d_axi_demux_mst (perf_cbi_d_axi_demux_if),
        .perf_l2_mst              (perf_l2_if),
        .perf_ifu_mst             (perf_ifu_if),
        .perf_bpu_mst             (perf_bpu_if),
        .perf_exu_mst             (perf_exu_if),
        .perf_lsu_mst             (perf_lsu_if),
        .perf_mmu_mst             (perf_mmu_if),
        .perf_l1i_mst             (perf_l1i_if),
        .perf_l1d_mst             (perf_l1d_if),
        .uart_irq_slv     (uart_irq),
        .gpio_irq_slv     (gpio_irq),
        .gtimer_irq_slv   (gtimer_irq)
    );

    axi_demux #(
        .GST_NUM  (2),
        .STG_FIFO_DEPTH (`SOC_BUS_STG_FIFO_DEPTH),
        .OST_DEPTH       (`SOC_BUS_OST_DEPTH),
`ifdef VERILATOR
        .GST_BASE ('{32'h40000000, 32'h80000000, 32'h00000000,
            32'h00000000, 32'h00000000}),
        .GST_SIZE ('{32'h40000000, 32'h02000000, 32'h00000000,
            32'h00000000, 32'h00000000})
`else
        .GST_BASE ('{32'h40000000, 32'h80000000}),
        .GST_SIZE ('{32'h40000000, 32'h02000000})
`endif
    ) u_mm_axi_demux(
        .clk                (clk),
        .rst_n              (rst_n),
        .host_axi4_aw_slv   (mm_aw),
        .host_axi4_w_slv    (mm_w),
        .host_axi4_b_mst    (mm_b),
        .host_axi4_ar_slv   (mm_ar),
        .host_axi4_r_mst    (mm_r),
        .gst_axi4_aw_msts   (mm_gst_aw),
        .gst_axi4_w_msts    (mm_gst_w),
        .gst_axi4_b_slvs    (mm_gst_b),
        .gst_axi4_ar_msts   (mm_gst_ar),
        .gst_axi4_r_slvs    (mm_gst_r),
        .perf_mst           (perf_mm_axi_demux_if)
    );

    axi_link u_ddr_axi_link(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (mm_gst_aw[0]),
        .host_axi4_w_slv  (mm_gst_w[0]),
        .host_axi4_b_mst  (mm_gst_b[0]),
        .host_axi4_ar_slv (mm_gst_ar[0]),
        .host_axi4_r_mst  (mm_gst_r[0]),
        .gst_axi4_aw_mst  (ddr_axi4_aw_mst),
        .gst_axi4_w_mst   (ddr_axi4_w_mst),
        .gst_axi4_b_slv   (ddr_axi4_b_slv),
        .gst_axi4_ar_mst  (ddr_axi4_ar_mst),
        .gst_axi4_r_slv   (ddr_axi4_r_slv)
    );

    axi_link u_flash_axi_link(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (mm_gst_aw[1]),
        .host_axi4_w_slv  (mm_gst_w[1]),
        .host_axi4_b_mst  (mm_gst_b[1]),
        .host_axi4_ar_slv (mm_gst_ar[1]),
        .host_axi4_r_mst  (mm_gst_r[1]),
        .gst_axi4_aw_mst  (flash_axi4_aw_mst),
        .gst_axi4_w_mst   (flash_axi4_w_mst),
        .gst_axi4_b_slv   (flash_axi4_b_slv),
        .gst_axi4_ar_mst  (flash_axi4_ar_mst),
        .gst_axi4_r_slv   (flash_axi4_r_slv)
    );

    peri u_peri(
        .clk         (clk),
        .rst_n       (rst_n),
        .apb_req_slv (peri_req),
        .apb_rsp_mst (peri_rsp),
        .uart_tx     (uart_tx),
        .uart_rx     (uart_rx),
        .gpio_in     (gpio_in),
        .gpio_out    (gpio_out),
        .gpio_oe     (gpio_oe),
        .pcm_inc_en  (pcm_inc_en),
        .uart_irq_mst(uart_irq),
        .gpio_irq_mst(gpio_irq),
        .gtimer_irq_mst(gtimer_irq)
    );

    assign perf_soc_if.pkt.cycle = 1'b1;
    assign perf_soc_if.pkt.soc_mm_aw_bp = mm_aw.vld && !mm_aw.rdy;
    assign perf_soc_if.pkt.soc_mm_w_bp = mm_w.vld && !mm_w.rdy;
    assign perf_soc_if.pkt.soc_mm_ar_bp = mm_ar.vld && !mm_ar.rdy;
    assign perf_soc_if.pkt.soc_mm_b_bp = mm_b.vld && !mm_b.rdy;
    assign perf_soc_if.pkt.soc_mm_r_bp = mm_r.vld && !mm_r.rdy;
    assign perf_soc_if.pkt.soc_ddr_aw_bp =
        ddr_axi4_aw_mst.vld && !ddr_axi4_aw_mst.rdy;
    assign perf_soc_if.pkt.soc_ddr_w_bp =
        ddr_axi4_w_mst.vld && !ddr_axi4_w_mst.rdy;
    assign perf_soc_if.pkt.soc_ddr_ar_bp =
        ddr_axi4_ar_mst.vld && !ddr_axi4_ar_mst.rdy;
    assign perf_soc_if.pkt.soc_ddr_b_bp =
        ddr_axi4_b_slv.vld && !ddr_axi4_b_slv.rdy;
    assign perf_soc_if.pkt.soc_ddr_r_bp =
        ddr_axi4_r_slv.vld && !ddr_axi4_r_slv.rdy;
    assign perf_soc_if.pkt.soc_flash_aw_bp =
        flash_axi4_aw_mst.vld && !flash_axi4_aw_mst.rdy;
    assign perf_soc_if.pkt.soc_flash_w_bp =
        flash_axi4_w_mst.vld && !flash_axi4_w_mst.rdy;
    assign perf_soc_if.pkt.soc_flash_ar_bp =
        flash_axi4_ar_mst.vld && !flash_axi4_ar_mst.rdy;
    assign perf_soc_if.pkt.soc_flash_b_bp =
        flash_axi4_b_slv.vld && !flash_axi4_b_slv.rdy;
    assign perf_soc_if.pkt.soc_flash_r_bp =
        flash_axi4_r_slv.vld && !flash_axi4_r_slv.rdy;
    assign perf_soc_if.pkt.peri_apb_bp =
        peri_req.psel && peri_req.penable && !peri_rsp.pready;

    pcm_perf u_pcm_perf(
        .clk                                           (clk),
        .rst_n                                         (rst_n),
        .inc_en                                        (pcm_inc_en),
        .perf_u_soc_slv                                (perf_soc_if),
        .perf_u_soc_u_mm_axi_demux_slv                 (perf_mm_axi_demux_if),
        .perf_u_soc_u_rv32g_cpu_slv                    (perf_rv32g_if),
        .perf_u_soc_u_rv32g_cpu_u_cbi_u_i_axi_demux_slv(perf_cbi_i_axi_demux_if),
        .perf_u_soc_u_rv32g_cpu_u_cbi_u_d_axi_demux_slv(perf_cbi_d_axi_demux_if),
        .perf_u_soc_u_rv32g_cpu_u_l2_slv               (perf_l2_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_ifu_slv       (perf_ifu_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_bpu_slv       (perf_bpu_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_mmu_slv       (perf_mmu_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_l1i_slv       (perf_l1i_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_l1d_slv       (perf_l1d_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_exu_slv       (perf_exu_if),
        .perf_u_soc_u_rv32g_cpu_u_hart_u_lsu_slv       (perf_lsu_if)
    );
endmodule
