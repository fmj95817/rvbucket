module l1(
    input logic       clk,
    input logic       rst_n,
    bti_req_if_t.slv  host_bti_req_slv,
    bti_rsp_if_t.mst  host_bti_rsp_mst,
    axi4_aw_if_t.mst  mem_axi4_aw_mst,
    axi4_w_if_t.mst   mem_axi4_w_mst,
    axi4_b_if_t.slv   mem_axi4_b_slv,
    axi4_ar_if_t.mst  mem_axi4_ar_mst,
    axi4_r_if_t.slv   mem_axi4_r_slv
);
    bti2axi u_bti2axi(
        .clk          (clk),
        .rst_n        (rst_n),
        .bti_req_slv  (host_bti_req_slv),
        .bti_rsp_mst  (host_bti_rsp_mst),
        .axi4_aw_mst  (mem_axi4_aw_mst),
        .axi4_w_mst   (mem_axi4_w_mst),
        .axi4_b_slv   (mem_axi4_b_slv),
        .axi4_ar_mst  (mem_axi4_ar_mst),
        .axi4_r_slv   (mem_axi4_r_slv)
    );
endmodule
