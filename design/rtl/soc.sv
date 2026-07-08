`include "spec/core/isa.svh"
module soc(
    input logic         clk,
    input logic         rst_n,
    output logic        uart_tx,
    input logic         uart_rx,
    input logic [23:0]  gpio_in,
    output logic [23:0] gpio_out,
    output logic [23:0] gpio_oe
);
    localparam FLASH_AW = 25;
    localparam DDR_AW = 28;

    apb_req_if_t peri_req();
    apb_rsp_if_t peri_rsp();
    bti_req_if_t flash_bti_req();
    bti_rsp_if_t flash_bti_rsp();
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
        .uart_irq_slv     (uart_irq),
        .gpio_irq_slv     (gpio_irq),
        .gtimer_irq_slv   (gtimer_irq)
    );

    axi_demux #(
        .GST_NUM  (2),
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
        .gst_axi4_r_slvs    (mm_gst_r)
    );

    axi_ddr #(
        .DDR_AW (DDR_AW)
    ) u_ddr(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (mm_gst_aw[0]),
        .axi4_w_slv   (mm_gst_w[0]),
        .axi4_b_mst   (mm_gst_b[0]),
        .axi4_ar_slv  (mm_gst_ar[0]),
        .axi4_r_mst   (mm_gst_r[0])
    );

    axi2bti u_flash_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (mm_gst_aw[1]),
        .axi4_w_slv   (mm_gst_w[1]),
        .axi4_b_mst   (mm_gst_b[1]),
        .axi4_ar_slv  (mm_gst_ar[1]),
        .axi4_r_mst   (mm_gst_r[1]),
        .bti_req_mst  (flash_bti_req),
        .bti_rsp_slv  (flash_bti_rsp)
    );

    bti_rom #(
        .BTI_AW (`RV_AW),
        .BTI_DW (`RV_XLEN),
        .ROM_AW (FLASH_AW)
    ) u_flash(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (flash_bti_req),
        .bti_rsp_mst (flash_bti_rsp)
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
        .uart_irq_mst(uart_irq),
        .gpio_irq_mst(gpio_irq),
        .gtimer_irq_mst(gtimer_irq)
    );
endmodule
