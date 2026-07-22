`include "spec/core/core.svh"
`include "spec/io/io.svh"

module io(
    input logic clk,
    input logic rst_n,
    apb_req_if_t.slv apb_req_slv,
    apb_rsp_if_t.mst apb_rsp_mst,
    axi4_aw_if_t.mst dma_axi4_aw_mst,
    axi4_w_if_t.mst  dma_axi4_w_mst,
    axi4_b_if_t.slv  dma_axi4_b_slv,
    axi4_ar_if_t.mst dma_axi4_ar_mst,
    axi4_r_if_t.slv  dma_axi4_r_slv,
    ext_irq_if_t.mst sdspi_irq_mst,
    sdspi_cmd_if_t.mst sdspi_cmd_mst,
    sdspi_data_if_t.slv sdspi_data_slv
);
`ifdef VERILATOR
    localparam logic [31:0] SDSPI_GST_BASE[4] = '{`IO_SUBSYS_BASE + `IO_SDSPI_OFFSET, 0, 0, 0};
    localparam logic [31:0] SDSPI_GST_SIZE[4] = '{`IO_SDSPI_SIZE, 0, 0, 0};
`else
    localparam logic [31:0] SDSPI_GST_BASE[1] = '{`IO_SUBSYS_BASE + `IO_SDSPI_OFFSET};
    localparam logic [31:0] SDSPI_GST_SIZE[1] = '{`IO_SDSPI_SIZE};
`endif

    apb_req_if_t sdspi_req[1]();
    apb_rsp_if_t sdspi_rsp[1]();

    apb_demux #(
        .GST_NUM(1),
        .GST_BASE(SDSPI_GST_BASE),
        .GST_SIZE(SDSPI_GST_SIZE)
    ) u_io_apb_demux(
        .host_apb_req_slv(apb_req_slv),
        .host_apb_rsp_mst(apb_rsp_mst),
        .gst_apb_req_msts(sdspi_req),
        .gst_apb_rsp_slvs(sdspi_rsp)
    );

    sdspi #(.BASE(`IO_SUBSYS_BASE + `IO_SDSPI_OFFSET)) u_sdspi(
        .clk(clk), .rst_n(rst_n),
        .apb_req_slv(sdspi_req[0]), .apb_rsp_mst(sdspi_rsp[0]),
        .dma_axi4_aw_mst(dma_axi4_aw_mst),
        .dma_axi4_w_mst(dma_axi4_w_mst),
        .dma_axi4_b_slv(dma_axi4_b_slv),
        .dma_axi4_ar_mst(dma_axi4_ar_mst),
        .dma_axi4_r_slv(dma_axi4_r_slv),
        .irq_mst(sdspi_irq_mst), .cmd_mst(sdspi_cmd_mst),
        .data_slv(sdspi_data_slv)
    );
endmodule
