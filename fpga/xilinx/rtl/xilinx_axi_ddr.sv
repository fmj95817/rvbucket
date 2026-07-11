module xilinx_axi_ddr #(
    parameter DDR_BASE = 32'h40000000,
    parameter AXI_AW = 30,
    parameter DDR_ADDR_WIDTH = 15,
    parameter DDR_DQ_WIDTH = 32,
    parameter DDR_DQS_WIDTH = 4,
    parameter DDR_DM_WIDTH = 4,
    parameter DDR_BA_WIDTH = 3
)(
    input logic                         sys_clk,
    input logic                         ref_clk,
    input logic                         rst_n,
    output logic                        ui_clk,
    output logic                        ui_rst_n,
    output logic                        init_calib_complete,

    inout logic [DDR_DQ_WIDTH-1:0]      ddr3_dq,
    inout logic [DDR_DQS_WIDTH-1:0]     ddr3_dqs_n,
    inout logic [DDR_DQS_WIDTH-1:0]     ddr3_dqs_p,
    output logic [DDR_ADDR_WIDTH-1:0]   ddr3_addr,
    output logic [DDR_BA_WIDTH-1:0]     ddr3_ba,
    output logic                        ddr3_ras_n,
    output logic                        ddr3_cas_n,
    output logic                        ddr3_we_n,
    output logic                        ddr3_reset_n,
    output logic                        ddr3_ck_p,
    output logic                        ddr3_ck_n,
    output logic                        ddr3_cke,
    output logic                        ddr3_cs_n,
    output logic [DDR_DM_WIDTH-1:0]     ddr3_dm,
    output logic                        ddr3_odt,

    axi4_aw_if_t.slv                    axi4_aw_slv,
    axi4_w_if_t.slv                     axi4_w_slv,
    axi4_b_if_t.mst                     axi4_b_mst,
    axi4_ar_if_t.slv                    axi4_ar_slv,
    axi4_r_if_t.mst                     axi4_r_mst
);
    logic ui_clk_sync_rst;
    logic mmcm_locked;
    logic [11:0] device_temp;
    logic ui_arst_n;
    logic mig_axi_rst_n;
    (* ASYNC_REG = "TRUE" *) logic [2:0] ui_rst_n_pipe;
    logic [31:0] awaddr_rel;
    logic [31:0] araddr_rel;

    assign ui_arst_n = rst_n && !ui_clk_sync_rst;
    assign mig_axi_rst_n = ui_rst_n_pipe[2];
    assign ui_rst_n = ui_rst_n_pipe[2];
    assign awaddr_rel = axi4_aw_slv.pkt.addr - DDR_BASE;
    assign araddr_rel = axi4_ar_slv.pkt.addr - DDR_BASE;

    always_ff @(posedge ui_clk or negedge ui_arst_n) begin
        if (!ui_arst_n) begin
            ui_rst_n_pipe <= 3'b0;
        end else begin
            ui_rst_n_pipe <= {ui_rst_n_pipe[1:0], 1'b1};
        end
    end

    rvbucket_ddr u_mig(
        .ddr3_dq             (ddr3_dq),
        .ddr3_dqs_n          (ddr3_dqs_n),
        .ddr3_dqs_p          (ddr3_dqs_p),
        .ddr3_addr           (ddr3_addr),
        .ddr3_ba             (ddr3_ba),
        .ddr3_ras_n          (ddr3_ras_n),
        .ddr3_cas_n          (ddr3_cas_n),
        .ddr3_we_n           (ddr3_we_n),
        .ddr3_reset_n        (ddr3_reset_n),
        .ddr3_ck_p           (ddr3_ck_p),
        .ddr3_ck_n           (ddr3_ck_n),
        .ddr3_cke            (ddr3_cke),
        .ddr3_cs_n           (ddr3_cs_n),
        .ddr3_dm             (ddr3_dm),
        .ddr3_odt            (ddr3_odt),
        .sys_clk_i           (sys_clk),
        .clk_ref_i           (ref_clk),
        .sys_rst             (rst_n),
        .ui_clk              (ui_clk),
        .ui_clk_sync_rst     (ui_clk_sync_rst),
        .ui_addn_clk_0       (),
        .ui_addn_clk_1       (),
        .ui_addn_clk_2       (),
        .ui_addn_clk_3       (),
        .ui_addn_clk_4       (),
        .mmcm_locked         (mmcm_locked),
        .device_temp         (device_temp),
        .aresetn             (mig_axi_rst_n),
        .app_sr_req          (1'b0),
        .app_ref_req         (1'b0),
        .app_zq_req          (1'b0),
        .app_sr_active       (),
        .app_ref_ack         (),
        .app_zq_ack          (),
        .init_calib_complete (init_calib_complete),

        .s_axi_awid          (axi4_aw_slv.pkt.id),
        .s_axi_awaddr        (awaddr_rel[AXI_AW-1:0]),
        .s_axi_awlen         (axi4_aw_slv.pkt.len),
        .s_axi_awsize        (axi4_aw_slv.pkt.size),
        .s_axi_awburst       (axi4_aw_slv.pkt.burst),
        .s_axi_awlock        (axi4_aw_slv.pkt.lock),
        .s_axi_awcache       (axi4_aw_slv.pkt.cache),
        .s_axi_awprot        (axi4_aw_slv.pkt.prot),
        .s_axi_awqos         (axi4_aw_slv.pkt.qos),
        .s_axi_awvalid       (axi4_aw_slv.vld),
        .s_axi_awready       (axi4_aw_slv.rdy),
        .s_axi_wdata         (axi4_w_slv.pkt.data),
        .s_axi_wstrb         (axi4_w_slv.pkt.strb),
        .s_axi_wlast         (axi4_w_slv.pkt.last),
        .s_axi_wvalid        (axi4_w_slv.vld),
        .s_axi_wready        (axi4_w_slv.rdy),
        .s_axi_bid           (axi4_b_mst.pkt.id),
        .s_axi_bresp         (axi4_b_mst.pkt.resp),
        .s_axi_bvalid        (axi4_b_mst.vld),
        .s_axi_bready        (axi4_b_mst.rdy),
        .s_axi_arid          (axi4_ar_slv.pkt.id),
        .s_axi_araddr        (araddr_rel[AXI_AW-1:0]),
        .s_axi_arlen         (axi4_ar_slv.pkt.len),
        .s_axi_arsize        (axi4_ar_slv.pkt.size),
        .s_axi_arburst       (axi4_ar_slv.pkt.burst),
        .s_axi_arlock        (axi4_ar_slv.pkt.lock),
        .s_axi_arcache       (axi4_ar_slv.pkt.cache),
        .s_axi_arprot        (axi4_ar_slv.pkt.prot),
        .s_axi_arqos         (axi4_ar_slv.pkt.qos),
        .s_axi_arvalid       (axi4_ar_slv.vld),
        .s_axi_arready       (axi4_ar_slv.rdy),
        .s_axi_rid           (axi4_r_mst.pkt.id),
        .s_axi_rdata         (axi4_r_mst.pkt.data),
        .s_axi_rresp         (axi4_r_mst.pkt.resp),
        .s_axi_rlast         (axi4_r_mst.pkt.last),
        .s_axi_rvalid        (axi4_r_mst.vld),
        .s_axi_rready        (axi4_r_mst.rdy)
    );
endmodule
