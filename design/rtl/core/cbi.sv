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
    apb_req_if_t.mst  cfg_apb_req_mst,
    apb_rsp_if_t.slv  cfg_apb_rsp_slv,
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
    apb_req_if_t cfg_req();
    apb_rsp_if_t cfg_rsp();

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
        .clk                (clk),
        .rst_n              (rst_n),
        .host_axi4_aw_slv   (hart_i_axi4_aw_slv),
        .host_axi4_w_slv    (hart_i_axi4_w_slv),
        .host_axi4_b_mst    (hart_i_axi4_b_mst),
        .host_axi4_ar_slv   (hart_i_axi4_ar_slv),
        .host_axi4_r_mst    (hart_i_axi4_r_mst),
        .gst_axi4_aw_msts   (i_aw),
        .gst_axi4_w_msts    (i_w),
        .gst_axi4_b_slvs    (i_b),
        .gst_axi4_ar_msts   (i_ar),
        .gst_axi4_r_slvs    (i_r)
    );

    axi_demux #(
        .GST_NUM    (D_GST_NUM),
        .GST_BASE   ('{32'h00000000, 32'h10000000, 32'h20000000,
            32'h40000000, 32'h30000000}),
        .GST_SIZE   ('{32'h00000800, 32'h00080000, 32'h00040000,
            32'h80000000, 32'h02000000})
    ) u_d_axi_demux(
        .clk                (clk),
        .rst_n              (rst_n),
        .host_axi4_aw_slv   (hart_d_axi4_aw_slv),
        .host_axi4_w_slv    (hart_d_axi4_w_slv),
        .host_axi4_b_mst    (hart_d_axi4_b_mst),
        .host_axi4_ar_slv   (hart_d_axi4_ar_slv),
        .host_axi4_r_mst    (hart_d_axi4_r_mst),
        .gst_axi4_aw_msts   (d_aw),
        .gst_axi4_w_msts    (d_w),
        .gst_axi4_b_slvs    (d_b),
        .gst_axi4_ar_msts   (d_ar),
        .gst_axi4_r_slvs    (d_r)
    );

    axi2bti u_boot_i_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (i_aw[0]),
        .axi4_w_slv   (i_w[0]),
        .axi4_b_mst   (i_b[0]),
        .axi4_ar_slv  (i_ar[0]),
        .axi4_r_mst   (i_r[0]),
        .bti_req_mst  (boot_i_req),
        .bti_rsp_slv  (boot_i_rsp)
    );

    axi2bti u_itcm_i_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (i_aw[1]),
        .axi4_w_slv   (i_w[1]),
        .axi4_b_mst   (i_b[1]),
        .axi4_ar_slv  (i_ar[1]),
        .axi4_r_mst   (i_r[1]),
        .bti_req_mst  (itcm_i_req),
        .bti_rsp_slv  (itcm_i_rsp)
    );

    axi_link u_mm_i_link(
        .host_axi4_aw_slv (i_aw[2]),
        .host_axi4_w_slv  (i_w[2]),
        .host_axi4_b_mst  (i_b[2]),
        .host_axi4_ar_slv (i_ar[2]),
        .host_axi4_r_mst  (i_r[2]),
        .gst_axi4_aw_mst  (mm_i_axi4_aw_mst),
        .gst_axi4_w_mst   (mm_i_axi4_w_mst),
        .gst_axi4_b_slv   (mm_i_axi4_b_slv),
        .gst_axi4_ar_mst  (mm_i_axi4_ar_mst),
        .gst_axi4_r_slv   (mm_i_axi4_r_slv)
    );

    axi2bti u_boot_d_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (d_aw[0]),
        .axi4_w_slv   (d_w[0]),
        .axi4_b_mst   (d_b[0]),
        .axi4_ar_slv  (d_ar[0]),
        .axi4_r_mst   (d_r[0]),
        .bti_req_mst  (boot_d_req),
        .bti_rsp_slv  (boot_d_rsp)
    );

    axi2bti u_itcm_d_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (d_aw[1]),
        .axi4_w_slv   (d_w[1]),
        .axi4_b_mst   (d_b[1]),
        .axi4_ar_slv  (d_ar[1]),
        .axi4_r_mst   (d_r[1]),
        .bti_req_mst  (itcm_d_req),
        .bti_rsp_slv  (itcm_d_rsp)
    );

    axi2bti u_dtcm_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (d_aw[2]),
        .axi4_w_slv   (d_w[2]),
        .axi4_b_mst   (d_b[2]),
        .axi4_ar_slv  (d_ar[2]),
        .axi4_r_mst   (d_r[2]),
        .bti_req_mst  (dtcm_req),
        .bti_rsp_slv  (dtcm_rsp)
    );

    axi_link u_mm_d_link(
        .host_axi4_aw_slv (d_aw[3]),
        .host_axi4_w_slv  (d_w[3]),
        .host_axi4_b_mst  (d_b[3]),
        .host_axi4_ar_slv (d_ar[3]),
        .host_axi4_r_mst  (d_r[3]),
        .gst_axi4_aw_mst  (mm_d_axi4_aw_mst),
        .gst_axi4_w_mst   (mm_d_axi4_w_mst),
        .gst_axi4_b_slv   (mm_d_axi4_b_slv),
        .gst_axi4_ar_mst  (mm_d_axi4_ar_mst),
        .gst_axi4_r_slv   (mm_d_axi4_r_slv)
    );

    axi2apb u_cfg_axi2apb(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (d_aw[4]),
        .axi4_w_slv   (d_w[4]),
        .axi4_b_mst   (d_b[4]),
        .axi4_ar_slv  (d_ar[4]),
        .axi4_r_mst   (d_r[4]),
        .apb_req_mst  (cfg_req),
        .apb_rsp_slv  (cfg_rsp)
    );

    bti_mux2 u_boot_bti_mux(
        .clk           (clk),
        .rst_n         (rst_n),
        .host0_req_slv (boot_i_req),
        .host0_rsp_mst (boot_i_rsp),
        .host1_req_slv (boot_d_req),
        .host1_rsp_mst (boot_d_rsp),
        .gst_req_mst   (boot_rom_bti_req_mst),
        .gst_rsp_slv   (boot_rom_bti_rsp_slv)
    );

    bti_link u_itcm_i_link(
        .host_bti_req_slv (itcm_i_req),
        .host_bti_rsp_mst (itcm_i_rsp),
        .gst_bti_req_mst  (itcm_i_bti_req_mst),
        .gst_bti_rsp_slv  (itcm_i_bti_rsp_slv)
    );

    bti_link u_itcm_d_link(
        .host_bti_req_slv (itcm_d_req),
        .host_bti_rsp_mst (itcm_d_rsp),
        .gst_bti_req_mst  (itcm_d_bti_req_mst),
        .gst_bti_rsp_slv  (itcm_d_bti_rsp_slv)
    );

    bti_link u_dtcm_link(
        .host_bti_req_slv (dtcm_req),
        .host_bti_rsp_mst (dtcm_rsp),
        .gst_bti_req_mst  (dtcm_bti_req_mst),
        .gst_bti_rsp_slv  (dtcm_bti_rsp_slv)
    );
    assign cfg_apb_req_mst.psel = cfg_req.psel;
    assign cfg_apb_req_mst.penable = cfg_req.penable;
    assign cfg_apb_req_mst.pkt = cfg_req.pkt;
    assign cfg_rsp.pready = cfg_apb_rsp_slv.pready;
    assign cfg_rsp.pkt = cfg_apb_rsp_slv.pkt;
endmodule
