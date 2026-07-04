`include "spec/core/isa.svh"
`include "core/boot.svh"

module soc(
    input logic  clk,
    input logic  rst_n,
    output logic uart_tx,
    input logic  uart_rx
);
    localparam BOOT_ROM_AW = `BOOT_ROM_WORD_AW + 2;
    localparam FLASH_AW = 20;
    localparam ITCM_AW = 17;
    localparam DTCM_AW = 18;

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
    bti_req_if_t mm_bti_req();
    bti_rsp_if_t mm_bti_rsp();
    axi4_aw_if_t mm_aw();
    axi4_w_if_t mm_w();
    axi4_b_if_t mm_b();
    axi4_ar_if_t mm_ar();
    axi4_r_if_t mm_r();

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
        mm_aw, mm_w, mm_b, mm_ar, mm_r
    );

    axi2bti u_mm_axi2bti(
        clk, rst_n,
        mm_aw, mm_w, mm_b, mm_ar, mm_r,
        mm_bti_req, mm_bti_rsp
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
        .bti_req_slv (mm_bti_req),
        .bti_rsp_mst (mm_bti_rsp)
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
        .uart_rx     (uart_rx)
    );
endmodule
