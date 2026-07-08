`include "spec/core/isa.svh"
`include "boot.svh"

module cbi(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  hart_i_axi4_aw_slv,
    axi4_w_if_t.slv   hart_i_axi4_w_slv,
    axi4_b_if_t.mst   hart_i_axi4_b_mst,
    axi4_ar_if_t.slv  hart_i_axi4_ar_slv,
    axi4_r_if_t.mst   hart_i_axi4_r_mst,
    axi4_aw_if_t.slv  hart_d_axi4_aw_slv,
    axi4_w_if_t.slv   hart_d_axi4_w_slv,
    axi4_b_if_t.mst   hart_d_axi4_b_mst,
    axi4_ar_if_t.slv  hart_d_axi4_ar_slv,
    axi4_r_if_t.mst   hart_d_axi4_r_mst,
    bti_req_if_t.mst  boot_rom_bti_req_mst,
    bti_rsp_if_t.slv  boot_rom_bti_rsp_slv,
    bti_req_if_t.mst  itcm_i_bti_req_mst,
    bti_rsp_if_t.slv  itcm_i_bti_rsp_slv,
    bti_req_if_t.mst  itcm_d_bti_req_mst,
    bti_rsp_if_t.slv  itcm_d_bti_rsp_slv,
    bti_req_if_t.mst  dtcm_bti_req_mst,
    bti_rsp_if_t.slv  dtcm_bti_rsp_slv,
    bti_req_if_t.mst  cfg_bti_req_mst,
    bti_rsp_if_t.slv  cfg_bti_rsp_slv,
    axi4_aw_if_t.mst  mm_i_axi4_aw_mst,
    axi4_w_if_t.mst   mm_i_axi4_w_mst,
    axi4_b_if_t.slv   mm_i_axi4_b_slv,
    axi4_ar_if_t.mst  mm_i_axi4_ar_mst,
    axi4_r_if_t.slv   mm_i_axi4_r_slv,
    axi4_aw_if_t.mst  mm_d_axi4_aw_mst,
    axi4_w_if_t.mst   mm_d_axi4_w_mst,
    axi4_b_if_t.slv   mm_d_axi4_b_slv,
    axi4_ar_if_t.mst  mm_d_axi4_ar_mst,
    axi4_r_if_t.slv   mm_d_axi4_r_slv
);
    localparam I_GST_NUM = 3;
    localparam D_GST_NUM = 5;

    axi4_aw_if_t i_aw[I_GST_NUM]();
    axi4_w_if_t i_w[I_GST_NUM]();
    axi4_b_if_t i_b[I_GST_NUM]();
    axi4_ar_if_t i_ar[I_GST_NUM]();
    axi4_r_if_t i_r[I_GST_NUM]();
    axi4_aw_if_t d_aw[D_GST_NUM]();
    axi4_w_if_t d_w[D_GST_NUM]();
    axi4_b_if_t d_b[D_GST_NUM]();
    axi4_ar_if_t d_ar[D_GST_NUM]();
    axi4_r_if_t d_r[D_GST_NUM]();

    bti_req_if_t boot_i_req();
    bti_rsp_if_t boot_i_rsp();
    bti_req_if_t boot_d_req();
    bti_rsp_if_t boot_d_rsp();
    bti_req_if_t itcm_i_req();
    bti_rsp_if_t itcm_i_rsp();
    bti_req_if_t itcm_d_req();
    bti_rsp_if_t itcm_d_rsp();
    bti_req_if_t dtcm_req();
    bti_rsp_if_t dtcm_rsp();
    bti_req_if_t cfg_req();
    bti_rsp_if_t cfg_rsp();

    axi_demux #(
        .GST_NUM    (I_GST_NUM),
`ifdef VERILATOR
        .GST_BASE   ('{32'h00000000, 32'h10000000, 32'h40000000,
            32'h00000000, 32'h00000000}),
        .GST_SIZE   ('{32'h00000800, 32'h00080000, 32'h80000000,
            32'h00000000, 32'h00000000})
`else
        .GST_BASE   ('{32'h00000000, 32'h10000000, 32'h40000000}),
        .GST_SIZE   ('{32'h00000800, 32'h00080000, 32'h80000000})
`endif
    ) u_i_axi_demux(
        clk, rst_n,
        hart_i_axi4_aw_slv, hart_i_axi4_w_slv, hart_i_axi4_b_mst,
        hart_i_axi4_ar_slv, hart_i_axi4_r_mst,
        i_aw, i_w, i_b, i_ar, i_r
    );

    axi_demux #(
        .GST_NUM    (D_GST_NUM),
        .GST_BASE   ('{32'h00000000, 32'h10000000, 32'h20000000,
            32'h40000000, 32'h30000000}),
        .GST_SIZE   ('{32'h00000800, 32'h00080000, 32'h00040000,
            32'h80000000, 32'h02000000})
    ) u_d_axi_demux(
        clk, rst_n,
        hart_d_axi4_aw_slv, hart_d_axi4_w_slv, hart_d_axi4_b_mst,
        hart_d_axi4_ar_slv, hart_d_axi4_r_mst,
        d_aw, d_w, d_b, d_ar, d_r
    );

    axi2bti u_boot_i_axi2bti(clk, rst_n, i_aw[0], i_w[0], i_b[0], i_ar[0], i_r[0], boot_i_req, boot_i_rsp);
    axi2bti u_itcm_i_axi2bti(clk, rst_n, i_aw[1], i_w[1], i_b[1], i_ar[1], i_r[1], itcm_i_req, itcm_i_rsp);
    axi_link u_mm_i_link(i_aw[2], i_w[2], i_b[2], i_ar[2], i_r[2],
        mm_i_axi4_aw_mst, mm_i_axi4_w_mst, mm_i_axi4_b_slv, mm_i_axi4_ar_mst, mm_i_axi4_r_slv);

    axi2bti u_boot_d_axi2bti(clk, rst_n, d_aw[0], d_w[0], d_b[0], d_ar[0], d_r[0], boot_d_req, boot_d_rsp);
    axi2bti u_itcm_d_axi2bti(clk, rst_n, d_aw[1], d_w[1], d_b[1], d_ar[1], d_r[1], itcm_d_req, itcm_d_rsp);
    axi2bti u_dtcm_axi2bti(clk, rst_n, d_aw[2], d_w[2], d_b[2], d_ar[2], d_r[2], dtcm_req, dtcm_rsp);
    axi_link u_mm_d_link(d_aw[3], d_w[3], d_b[3], d_ar[3], d_r[3],
        mm_d_axi4_aw_mst, mm_d_axi4_w_mst, mm_d_axi4_b_slv, mm_d_axi4_ar_mst, mm_d_axi4_r_slv);
    axi2bti u_cfg_axi2bti(clk, rst_n, d_aw[4], d_w[4], d_b[4], d_ar[4], d_r[4], cfg_req, cfg_rsp);

    bti_mux2 u_boot_bti_mux(
        clk, rst_n,
        boot_i_req, boot_i_rsp, boot_d_req, boot_d_rsp,
        boot_rom_bti_req_mst, boot_rom_bti_rsp_slv
    );

    bti_link u_itcm_i_link(itcm_i_req, itcm_i_rsp, itcm_i_bti_req_mst, itcm_i_bti_rsp_slv);
    bti_link u_itcm_d_link(itcm_d_req, itcm_d_rsp, itcm_d_bti_req_mst, itcm_d_bti_rsp_slv);
    bti_link u_dtcm_link(dtcm_req, dtcm_rsp, dtcm_bti_req_mst, dtcm_bti_rsp_slv);
    bti_link u_cfg_link(cfg_req, cfg_rsp, cfg_bti_req_mst, cfg_bti_rsp_slv);
endmodule
