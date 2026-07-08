`include "spec/core/isa.svh"
`include "core/boot.svh"

module soc(
    input logic  clk,
    input logic  rst_n,
    output logic uart_tx,
    input logic  uart_rx
);
    localparam BOOT_ROM_AW = `BOOT_ROM_WORD_AW + 2;
    localparam FLASH_AW = 25;
    localparam ITCM_AW = 19;
    localparam DTCM_AW = 18;
    localparam DDR_AW = 28;

    bti_req_if_t boot_rom_req();
    bti_rsp_if_t boot_rom_rsp();
    bti_req_if_t itcm_i_req();
    bti_rsp_if_t itcm_i_rsp();
    bti_req_if_t itcm_d_req();
    bti_rsp_if_t itcm_d_rsp();
    bti_req_if_t dtcm_req();
    bti_rsp_if_t dtcm_rsp();
    bti_req_if_t cfg_req();
    bti_rsp_if_t cfg_rsp();
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

    tri boot_rom_cs;
    tri [`BOOT_ROM_WORD_AW-1:0] boot_rom_addr;
    tri [`RV_XLEN-1:0] boot_rom_data;

    rv32g u_rv32g(
        clk, rst_n,
        boot_rom_req, boot_rom_rsp,
        itcm_i_req, itcm_i_rsp,
        itcm_d_req, itcm_d_rsp,
        dtcm_req, dtcm_rsp,
        cfg_req, cfg_rsp,
        mm_aw, mm_w, mm_b, mm_ar, mm_r,
        uart_irq
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
        clk, rst_n,
        mm_aw, mm_w, mm_b, mm_ar, mm_r,
        mm_gst_aw, mm_gst_w, mm_gst_b, mm_gst_ar, mm_gst_r
    );

    axi_sram #(
        .SRAM_AW (DDR_AW)
    ) u_ddr(
        clk, rst_n,
        mm_gst_aw[0], mm_gst_w[0], mm_gst_b[0],
        mm_gst_ar[0], mm_gst_r[0]
    );

    axi2bti u_flash_axi2bti(
        clk, rst_n,
        mm_gst_aw[1], mm_gst_w[1], mm_gst_b[1], mm_gst_ar[1], mm_gst_r[1],
        flash_bti_req, flash_bti_rsp
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

    bti_dp_sram #(
        .BTI_AW  (`RV_AW),
        .BTI_DW  (`RV_XLEN),
        .SRAM_AW (ITCM_AW)
    ) u_itcm(
        .clk           (clk),
        .rst_n         (rst_n),
        .bti_r_req_slv (itcm_i_req),
        .bti_r_rsp_mst (itcm_i_rsp),
        .bti_w_req_slv (itcm_d_req),
        .bti_w_rsp_mst (itcm_d_rsp)
    );

    bti_sram #(
        .BTI_AW  (`RV_AW),
        .BTI_DW  (`RV_XLEN),
        .SRAM_AW (DTCM_AW)
    ) u_dtcm(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (dtcm_req),
        .bti_rsp_mst (dtcm_rsp)
    );

    peri u_peri(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (cfg_req),
        .bti_rsp_mst (cfg_rsp),
        .uart_tx     (uart_tx),
        .uart_rx     (uart_rx),
        .uart_irq_mst(uart_irq)
    );
endmodule
