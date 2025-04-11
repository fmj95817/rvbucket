`include "isa.svh"

module sim_top;
    localparam ITCM_AW = 15;
    localparam DTCM_AW = 15;

    tri                    clk;
    tri                    rst_n;

    bti_req_if_t #(`RV_AW, `RV_XLEN) itcm_bti_req_if();
    bti_rsp_if_t #(`RV_XLEN)         itcm_bti_rsp_if();
    bti_req_if_t #(`RV_AW, `RV_XLEN) dtcm_bti_req_if();
    bti_rsp_if_t #(`RV_XLEN)         dtcm_bti_rsp_if();

    clk_rst u_clk_rst(
        .clk              (clk),
        .rst_n            (rst_n)
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
        .SRAM_AW          (DTCM_AW)
    ) u_dtcm(
        .clk              (clk),
        .rst_n            (rst_n),
        .bti_req_slv      (dtcm_bti_req_if),
        .bti_rsp_mst      (dtcm_bti_rsp_if)
    );

    rv32i u_rv32i(
        .clk              (clk),
        .rst_n            (rst_n),
        .itcm_bti_req_mst (itcm_bti_req_if),
        .itcm_bti_rsp_slv (itcm_bti_rsp_if),
        .dtcm_bti_req_mst (dtcm_bti_req_if),
        .dtcm_bti_rsp_slv (dtcm_bti_rsp_if)
    );

    initial begin
        string path;
        if ($value$plusargs ("program=%s", path)) begin
            $readmemh(path, u_itcm.u_rom.data);
        end
    end
endmodule