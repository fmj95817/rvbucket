`include "isa.svh"
`include "boot.svh"

module soc(
    input logic       clk,
    input logic       rst_n
);
    localparam BTI_GST_SEL_AW = 8;
    localparam I_BTI_GST_NUM = 2;
    localparam D_BTI_GST_NUM = 4;

    localparam BOOT_ROM_AW = `BOOT_ROM_WORD_AW + 2;
    localparam FLASH_AW = 20;
    localparam ITCM_AW = 17;
    localparam DTCM_AW = 16;
    localparam UART_AW = 2;

    localparam BOOT_ROM_SEL = 8'h40;
    localparam FLASH_SEL = 8'h80;
    localparam ITCM_SEL = 8'h10;
    localparam DTCM_SEL = 8'h20;
    localparam UART_SEL = 8'h30;

    bti_req_if_t #(`RV_AW, `RV_XLEN) i_bti_req_if();
    bti_rsp_if_t #(`RV_XLEN)         i_bti_rsp_if();
    bti_req_if_t #(`RV_AW, `RV_XLEN) d_bti_req_if();
    bti_rsp_if_t #(`RV_XLEN)         d_bti_rsp_if();

    bti_req_if_t #(`RV_AW, `RV_XLEN) i_bti_req_if_arr[I_BTI_GST_NUM]();
    bti_rsp_if_t #(`RV_XLEN)         i_bti_rsp_if_arr[I_BTI_GST_NUM]();
    bti_req_if_t #(`RV_AW, `RV_XLEN) d_bti_req_if_arr[D_BTI_GST_NUM]();
    bti_rsp_if_t #(`RV_XLEN)         d_bti_rsp_if_arr[D_BTI_GST_NUM]();

    tri                              boot_rom_cs;
    tri [`BOOT_ROM_WORD_AW-1:0]      boot_rom_addr;
    tri [`RV_XLEN-1:0]               boot_rom_data;

    rv32i u_rv32i(
        .clk              (clk),
        .rst_n            (rst_n),
        .i_bti_req_mst    (i_bti_req_if),
        .i_bti_rsp_slv    (i_bti_rsp_if),
        .d_bti_req_mst    (d_bti_req_if),
        .d_bti_rsp_slv    (d_bti_rsp_if)
    );

    boot_rom u_boot_rom(
        .clk              (clk),
        .cs               (boot_rom_cs),
        .addr             (boot_rom_addr),
        .data             (boot_rom_data)
    );

    bti_to_rom #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .ROM_AW           (BOOT_ROM_AW)
    ) u_bti_to_boot_rom(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (i_bti_req_if_arr[0]),
        .bti_rsp_mst      (i_bti_rsp_if_arr[0]),
        .cs               (boot_rom_cs),
        .addr             (boot_rom_addr),
        .data             (boot_rom_data)
    );

    bti_rom #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .ROM_AW           (FLASH_AW)
    ) u_flash(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (d_bti_req_if_arr[0]),
        .bti_rsp_mst      (d_bti_rsp_if_arr[0])
    );

    bti_dp_sram #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .SRAM_AW          (ITCM_AW)
    ) u_itcm(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_r_req_slv    (i_bti_req_if_arr[1]),
        .bti_r_rsp_mst    (i_bti_rsp_if_arr[1]),
        .bti_w_req_slv    (d_bti_req_if_arr[1]),
        .bti_w_rsp_mst    (d_bti_rsp_if_arr[1])
    );

    bti_sram #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .SRAM_AW          (DTCM_AW)
    ) u_dtcm(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (d_bti_req_if_arr[2]),
        .bti_rsp_mst      (d_bti_rsp_if_arr[2])
    );

    bti_uart #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .UART_AW          (UART_AW)
    ) u_uart(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (d_bti_req_if_arr[3]),
        .bti_rsp_mst      (d_bti_rsp_if_arr[3])
    );

    bti_demux #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .GST_SEL_AW       (BTI_GST_SEL_AW),
        .GST_NUM          (I_BTI_GST_NUM),
        .GST_SEL          ('{BOOT_ROM_SEL, ITCM_SEL}),
        .GST_AW           ('{BOOT_ROM_AW, ITCM_AW})
    ) u_i_bti_demux(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (i_bti_req_if),
        .host_bti_rsp_mst (i_bti_rsp_if),
        .gst_bti_req_msts (i_bti_req_if_arr),
        .gst_bti_rsp_slvs (i_bti_rsp_if_arr)
    );

    bti_demux #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .GST_SEL_AW       (BTI_GST_SEL_AW),
        .GST_NUM          (D_BTI_GST_NUM),
        .GST_SEL          ('{FLASH_SEL, ITCM_SEL, DTCM_SEL, UART_SEL}),
        .GST_AW           ('{FLASH_AW, ITCM_AW, DTCM_AW, UART_AW})
    ) u_d_bti_demux(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (d_bti_req_if),
        .host_bti_rsp_mst (d_bti_rsp_if),
        .gst_bti_req_msts (d_bti_req_if_arr),
        .gst_bti_rsp_slvs (d_bti_rsp_if_arr)
    );

endmodule