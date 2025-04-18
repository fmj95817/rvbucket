`include "isa.svh"

module soc(
    input logic       clk,
    input logic       rst_n
);
    localparam ITCM_AW = 15;

    localparam DTCM_NUM = 3;
    localparam DTCM1_AW = 14;
    localparam DTCM2_AW = 15;
    localparam DTCM3_AW = 13;

    bti_req_if_t #(`RV_AW, `RV_XLEN) itcm_bti_req_if();
    bti_rsp_if_t #(`RV_XLEN)         itcm_bti_rsp_if();
    bti_req_if_t #(`RV_AW, `RV_XLEN) dtcm_bti_req_if();
    bti_rsp_if_t #(`RV_XLEN)         dtcm_bti_rsp_if();

    bti_req_if_t #(`RV_AW, `RV_XLEN) dtcm_bti_req_if_arr[DTCM_NUM]();
    bti_rsp_if_t #(`RV_XLEN)         dtcm_bti_rsp_if_arr[DTCM_NUM]();

    rv32i u_rv32i(
        .clk              (clk),
        .rst_n            (rst_n),
        .itcm_bti_req_mst (itcm_bti_req_if),
        .itcm_bti_rsp_slv (itcm_bti_rsp_if),
        .dtcm_bti_req_mst (dtcm_bti_req_if),
        .dtcm_bti_rsp_slv (dtcm_bti_rsp_if)
    );

    bti_rom #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .ROM_AW           (ITCM_AW)
    ) u_itcm(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (itcm_bti_req_if),
        .bti_rsp_mst      (itcm_bti_rsp_if)
    );

    bti_sram #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .SRAM_AW          (DTCM1_AW)
    ) u_dtcm1(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (dtcm_bti_req_if_arr[0]),
        .bti_rsp_mst      (dtcm_bti_rsp_if_arr[0])
    );

    bti_sram #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .SRAM_AW          (DTCM2_AW)
    ) u_dtcm2(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (dtcm_bti_req_if_arr[1]),
        .bti_rsp_mst      (dtcm_bti_rsp_if_arr[1])
    );

    bti_sram #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .SRAM_AW          (DTCM3_AW)
    ) u_dtcm3(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (dtcm_bti_req_if_arr[2]),
        .bti_rsp_mst      (dtcm_bti_rsp_if_arr[2])
    );

    bti_demux #(
        .BTI_AW           (`RV_AW),
        .BTI_DW           (`RV_XLEN),
        .GST_SEL_AW       (8),
        .GST_NUM          (DTCM_NUM),
        .GST_SEL_ADDRS    ({8'h0, 8'h2, 8'h5}),
        .GST_VLD_AW       ({DTCM1_AW, DTCM2_AW, DTCM3_AW})
    ) u_bti_demux(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (dtcm_bti_req_if),
        .host_bti_rsp_mst (dtcm_bti_rsp_if),
        .gst_bti_req_msts (dtcm_bti_req_if_arr),
        .gst_bti_rsp_slvs (dtcm_bti_rsp_if_arr)
    );

endmodule