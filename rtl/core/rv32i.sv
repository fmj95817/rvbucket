`include "bti.svh"

module rv32i(
    input logic       clk,
    input logic       rst_n,
    bti_req_if_t.mst  itcm_bti_req_mst,
    bti_rsp_if_t.slv  itcm_bti_rsp_slv,
    bti_req_if_t.mst  dtcm_bti_req_mst,
    bti_rsp_if_t.slv  dtcm_bti_rsp_slv
);
    fch_req_if_t fch_req_if();
    fch_rsp_if_t fch_rsp_if();
    ex_req_if_t  ex_req_if();
    ex_rsp_if_t  ex_rsp_if();
    ldst_req_if_t ldst_req_if();
    ldst_rsp_if_t ldst_rsp_if();

    ifu u_ifu(
        .clk              (clk),
        .rst_n            (rst_n),
        .fch_req_mst      (fch_req_if),
        .fch_rsp_slv      (fch_rsp_if),
        .ex_req_mst       (ex_req_if),
        .ex_rsp_slv       (ex_rsp_if)
    );

    exu u_exu(
        .clk              (clk),
        .rst_n            (rst_n),
        .ex_req_slv       (ex_req_if),
        .ex_rsp_mst       (ex_rsp_if),
        .ldst_req_mst     (ldst_req_if),
        .ldst_rsp_slv     (ldst_rsp_if)
    );

    biu u_biu(
        .clk              (clk),
        .rst_n            (rst_n),
        .fch_req_slv      (fch_req_if),
        .fch_rsp_mst      (fch_rsp_if),
        .ldst_req_slv     (ldst_req_if),
        .ldst_rsp_mst     (ldst_rsp_if),
        .itcm_bti_req_mst (itcm_bti_req_mst),
        .itcm_bti_rsp_slv (itcm_bti_rsp_slv),
        .dtcm_bti_req_mst (dtcm_bti_req_mst),
        .dtcm_bti_rsp_slv (dtcm_bti_rsp_slv)
    );

endmodule