`include "spec/core/isa.svh"
`include "spec/core/core.svh"
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
    apb_req_if_t.mst  peri_apb_req_mst,
    apb_rsp_if_t.slv  peri_apb_rsp_slv,
    apb_req_if_t.mst  io_apb_req_mst,
    apb_rsp_if_t.slv  io_apb_rsp_slv,
    apb_req_if_t.mst  aclint_apb_req_mst,
    apb_rsp_if_t.slv  aclint_apb_rsp_slv,
    apb_req_if_t.mst  plic_apb_req_mst,
    apb_rsp_if_t.slv  plic_apb_rsp_slv,
    axi4_aw_if_t.mst  mm_i_axi4_aw_mst,
    axi4_w_if_t.mst   mm_i_axi4_w_mst,
    axi4_b_if_t.slv   mm_i_axi4_b_slv,
    axi4_ar_if_t.mst  mm_i_axi4_ar_mst,
    axi4_r_if_t.slv   mm_i_axi4_r_slv,
    axi4_aw_if_t.mst  mm_d_axi4_aw_mst,
    axi4_w_if_t.mst   mm_d_axi4_w_mst,
    axi4_b_if_t.slv   mm_d_axi4_b_slv,
    axi4_ar_if_t.mst  mm_d_axi4_ar_mst,
    axi4_r_if_t.slv   mm_d_axi4_r_slv,
    perf_axi_demux_if_t.mst perf_i_axi_demux_mst,
    perf_axi_demux_if_t.mst perf_d_axi_demux_mst
);
    localparam I_GST_NUM = 2;
    localparam D_GST_NUM = 3;
    localparam logic [31:0] BOOT_ROM_SIZE =
        32'd1 << (`BOOT_ROM_WORD_AW + 2);

    axi4_aw_if_t i_aw[I_GST_NUM]();
    axi4_w_if_t i_w[I_GST_NUM]();
    axi4_b_if_t i_b[I_GST_NUM]();
    axi4_b_if_t i_b_gst[I_GST_NUM]();
    axi4_ar_if_t i_ar[I_GST_NUM]();
    axi4_r_if_t i_r[I_GST_NUM]();
    axi4_r_if_t i_r_gst[I_GST_NUM]();
    axi4_aw_if_t d_aw[D_GST_NUM]();
    axi4_w_if_t d_w[D_GST_NUM]();
    axi4_b_if_t d_b[D_GST_NUM]();
    axi4_b_if_t d_b_gst[D_GST_NUM]();
    axi4_ar_if_t d_ar[D_GST_NUM]();
    axi4_r_if_t d_r[D_GST_NUM]();
    axi4_r_if_t d_r_gst[D_GST_NUM]();

    bti_req_if_t boot_i_req();
    bti_rsp_if_t boot_i_rsp();
    bti_req_if_t boot_d_req();
    bti_rsp_if_t boot_d_rsp();
    apb_req_if_t cfg_req();
    apb_rsp_if_t cfg_rsp();
    apb_req_if_t cfg_gst_req[4]();
    apb_rsp_if_t cfg_gst_rsp[4]();

    for (genvar i = 0; i < I_GST_NUM; i++) begin : gen_i_rsp_slice
        axi_rsp_reg_slice u_rsp_slice(
            .clk             (clk),
            .rst_n           (rst_n),
            .host_axi4_b_mst (i_b[i]),
            .host_axi4_r_mst (i_r[i]),
            .gst_axi4_b_slv  (i_b_gst[i]),
            .gst_axi4_r_slv  (i_r_gst[i])
        );
    end

    for (genvar i = 0; i < D_GST_NUM; i++) begin : gen_d_rsp_slice
        axi_rsp_reg_slice u_rsp_slice(
            .clk             (clk),
            .rst_n           (rst_n),
            .host_axi4_b_mst (d_b[i]),
            .host_axi4_r_mst (d_r[i]),
            .gst_axi4_b_slv  (d_b_gst[i]),
            .gst_axi4_r_slv  (d_r_gst[i])
        );
    end

    axi_demux #(
        .GST_NUM    (I_GST_NUM),
        .STG_FIFO_DEPTH (`CORE_BUS_STG_FIFO_DEPTH),
        .OST_DEPTH       (`CORE_BUS_OST_DEPTH),
`ifdef VERILATOR
        .GST_BASE   ('{32'h00000000, 32'h40000000, 32'h00000000,
            32'h00000000, 32'h00000000}),
        .GST_SIZE   ('{BOOT_ROM_SIZE, 32'h80000000, 32'h00000000,
            32'h00000000, 32'h00000000})
`else
        .GST_BASE   ('{32'h00000000, 32'h40000000}),
        .GST_SIZE   ('{BOOT_ROM_SIZE, 32'h80000000})
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
        .gst_axi4_r_slvs    (i_r),
        .perf_mst           (perf_i_axi_demux_mst)
    );

    axi_demux #(
        .GST_NUM    (D_GST_NUM),
        .STG_FIFO_DEPTH (`CORE_BUS_STG_FIFO_DEPTH),
        .OST_DEPTH       (`CORE_BUS_OST_DEPTH),
`ifdef VERILATOR
        .GST_BASE   ('{32'h00000000, 32'h40000000, 32'h30000000,
            32'h00000000, 32'h00000000}),
        .GST_SIZE   ('{BOOT_ROM_SIZE, 32'h80000000, 32'h02000000,
            32'h00000000, 32'h00000000})
`else
        .GST_BASE   ('{32'h00000000, 32'h40000000, 32'h30000000}),
        .GST_SIZE   ('{BOOT_ROM_SIZE, 32'h80000000, 32'h02000000})
`endif
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
        .gst_axi4_r_slvs    (d_r),
        .perf_mst           (perf_d_axi_demux_mst)
    );

    axi2bti #(
        .STG_FIFO_DEPTH (`CORE_BUS_STG_FIFO_DEPTH),
        .OST_DEPTH      (`CORE_BUS_OST_DEPTH)
    ) u_boot_i_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (i_aw[0]),
        .axi4_w_slv   (i_w[0]),
        .axi4_b_mst   (i_b_gst[0]),
        .axi4_ar_slv  (i_ar[0]),
        .axi4_r_mst   (i_r_gst[0]),
        .bti_req_mst  (boot_i_req),
        .bti_rsp_slv  (boot_i_rsp)
    );

    axi_link u_mm_i_link(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (i_aw[1]),
        .host_axi4_w_slv  (i_w[1]),
        .host_axi4_b_mst  (i_b_gst[1]),
        .host_axi4_ar_slv (i_ar[1]),
        .host_axi4_r_mst  (i_r_gst[1]),
        .gst_axi4_aw_mst  (mm_i_axi4_aw_mst),
        .gst_axi4_w_mst   (mm_i_axi4_w_mst),
        .gst_axi4_b_slv   (mm_i_axi4_b_slv),
        .gst_axi4_ar_mst  (mm_i_axi4_ar_mst),
        .gst_axi4_r_slv   (mm_i_axi4_r_slv)
    );

    axi2bti #(
        .STG_FIFO_DEPTH (`CORE_BUS_STG_FIFO_DEPTH),
        .OST_DEPTH      (`CORE_BUS_OST_DEPTH)
    ) u_boot_d_axi2bti(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (d_aw[0]),
        .axi4_w_slv   (d_w[0]),
        .axi4_b_mst   (d_b_gst[0]),
        .axi4_ar_slv  (d_ar[0]),
        .axi4_r_mst   (d_r_gst[0]),
        .bti_req_mst  (boot_d_req),
        .bti_rsp_slv  (boot_d_rsp)
    );

    axi_link u_mm_d_link(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (d_aw[1]),
        .host_axi4_w_slv  (d_w[1]),
        .host_axi4_b_mst  (d_b_gst[1]),
        .host_axi4_ar_slv (d_ar[1]),
        .host_axi4_r_mst  (d_r_gst[1]),
        .gst_axi4_aw_mst  (mm_d_axi4_aw_mst),
        .gst_axi4_w_mst   (mm_d_axi4_w_mst),
        .gst_axi4_b_slv   (mm_d_axi4_b_slv),
        .gst_axi4_ar_mst  (mm_d_axi4_ar_mst),
        .gst_axi4_r_slv   (mm_d_axi4_r_slv)
    );

    axi2apb u_cfg_axi2apb(
        .clk          (clk),
        .rst_n        (rst_n),
        .axi4_aw_slv  (d_aw[2]),
        .axi4_w_slv   (d_w[2]),
        .axi4_b_mst   (d_b_gst[2]),
        .axi4_ar_slv  (d_ar[2]),
        .axi4_r_mst   (d_r_gst[2]),
        .apb_req_mst  (cfg_req),
        .apb_rsp_slv  (cfg_rsp)
    );

    apb_demux #(
        .GST_NUM  (4),
        .GST_BASE ('{`PERI_BASE, `IO_SUBSYS_BASE, `ACLINT_BASE, `PLIC_BASE}),
        .GST_SIZE ('{`PERI_SIZE, `IO_SUBSYS_SIZE, `ACLINT_SIZE, `PLIC_SIZE})
    ) u_cfg_apb_demux(
        .host_apb_req_slv (cfg_req),
        .host_apb_rsp_mst (cfg_rsp),
        .gst_apb_req_msts (cfg_gst_req),
        .gst_apb_rsp_slvs (cfg_gst_rsp)
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

    assign peri_apb_req_mst.psel = cfg_gst_req[0].psel;
    assign peri_apb_req_mst.penable = cfg_gst_req[0].penable;
    assign peri_apb_req_mst.pkt = cfg_gst_req[0].pkt;
    assign cfg_gst_rsp[0].pready = peri_apb_rsp_slv.pready;
    assign cfg_gst_rsp[0].pkt = peri_apb_rsp_slv.pkt;

    assign io_apb_req_mst.psel = cfg_gst_req[1].psel;
    assign io_apb_req_mst.penable = cfg_gst_req[1].penable;
    assign io_apb_req_mst.pkt = cfg_gst_req[1].pkt;
    assign cfg_gst_rsp[1].pready = io_apb_rsp_slv.pready;
    assign cfg_gst_rsp[1].pkt = io_apb_rsp_slv.pkt;

    assign aclint_apb_req_mst.psel = cfg_gst_req[2].psel;
    assign aclint_apb_req_mst.penable = cfg_gst_req[2].penable;
    assign aclint_apb_req_mst.pkt = cfg_gst_req[2].pkt;
    assign cfg_gst_rsp[2].pready = aclint_apb_rsp_slv.pready;
    assign cfg_gst_rsp[2].pkt = aclint_apb_rsp_slv.pkt;

    assign plic_apb_req_mst.psel = cfg_gst_req[3].psel;
    assign plic_apb_req_mst.penable = cfg_gst_req[3].penable;
    assign plic_apb_req_mst.pkt = cfg_gst_req[3].pkt;
    assign cfg_gst_rsp[3].pready = plic_apb_rsp_slv.pready;
    assign cfg_gst_rsp[3].pkt = plic_apb_rsp_slv.pkt;

endmodule
