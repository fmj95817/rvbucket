module fpga_top #(
    parameter DDR_ADDR_WIDTH = 15,
    parameter DDR_DQ_WIDTH = 32,
    parameter DDR_DQS_WIDTH = 4,
    parameter DDR_DM_WIDTH = 4,
    parameter DDR_BA_WIDTH = 3,
    parameter FLASH_AW = 32
)(
    input logic                         clk,
    input logic                         rst_n,
    output logic                        uart_tx,
    input logic                         uart_rx,
    output logic [7:0]                  led,

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

    inout logic [3:0]                   flash_dq,
    output logic                        flash_ce_n
);
    logic clk_50m;
    logic ddr_ref_clk;
    logic clkgen_locked;
    logic soc_clk;
    logic soc_rst_n;
    logic soc_rst_n_gated;
    logic soc_start_done;
    logic soc_uart_tx;
    logic ddr_rst_n;
    logic ddr_init_calib_complete;
    (* ASYNC_REG = "TRUE" *) logic [2:0] ddr_rst_n_pipe;
    logic [15:0] soc_start_cnt;
    logic [23:0] gpio_in;
    logic [23:0] gpio_out;
    logic [23:0] gpio_oe;

    axi4_aw_if_t ddr_axi4_aw();
    axi4_w_if_t ddr_axi4_w();
    axi4_b_if_t ddr_axi4_b();
    axi4_ar_if_t ddr_axi4_ar();
    axi4_r_if_t ddr_axi4_r();
    axi4_aw_if_t flash_axi4_aw();
    axi4_w_if_t flash_axi4_w();
    axi4_b_if_t flash_axi4_b();
    axi4_ar_if_t flash_axi4_ar();
    axi4_r_if_t flash_axi4_r();

    assign gpio_in = 24'h100000;
    assign ddr_rst_n = ddr_rst_n_pipe[2];
    assign soc_rst_n_gated = soc_rst_n && soc_start_done;
    assign uart_tx = soc_uart_tx;
    assign led = gpio_out[7:0];

    always_ff @(posedge clk_50m or negedge rst_n) begin
        if (!rst_n) begin
            ddr_rst_n_pipe <= 3'b0;
        end else begin
            ddr_rst_n_pipe <= {ddr_rst_n_pipe[1:0], clkgen_locked};
        end
    end

    always_ff @(posedge soc_clk or negedge soc_rst_n) begin
        if (!soc_rst_n) begin
            soc_start_cnt <= '0;
            soc_start_done <= 1'b0;
        end else if (!soc_start_done) begin
            soc_start_cnt <= soc_start_cnt + 1'b1;
            if (&soc_start_cnt) begin
                soc_start_done <= 1'b1;
            end
        end
    end

    rvbucket_clk_wiz u_clk_wiz(
        .clk_out1   (clk_50m),
        .clk_out2   (ddr_ref_clk),
        .resetn     (rst_n),
        .locked     (clkgen_locked),
        .clk_in1    (clk)
    );

    xilinx_axi_ddr #(
        .DDR_ADDR_WIDTH (DDR_ADDR_WIDTH),
        .DDR_DQ_WIDTH   (DDR_DQ_WIDTH),
        .DDR_DQS_WIDTH  (DDR_DQS_WIDTH),
        .DDR_DM_WIDTH   (DDR_DM_WIDTH),
        .DDR_BA_WIDTH   (DDR_BA_WIDTH)
    ) u_ddr(
        .sys_clk                 (clk_50m),
        .ref_clk                 (ddr_ref_clk),
        .rst_n                   (ddr_rst_n),
        .ui_clk                  (soc_clk),
        .ui_rst_n                (soc_rst_n),
        .init_calib_complete     (ddr_init_calib_complete),
        .ddr3_dq                 (ddr3_dq),
        .ddr3_dqs_n              (ddr3_dqs_n),
        .ddr3_dqs_p              (ddr3_dqs_p),
        .ddr3_addr               (ddr3_addr),
        .ddr3_ba                 (ddr3_ba),
        .ddr3_ras_n              (ddr3_ras_n),
        .ddr3_cas_n              (ddr3_cas_n),
        .ddr3_we_n               (ddr3_we_n),
        .ddr3_reset_n            (ddr3_reset_n),
        .ddr3_ck_p               (ddr3_ck_p),
        .ddr3_ck_n               (ddr3_ck_n),
        .ddr3_cke                (ddr3_cke),
        .ddr3_cs_n               (ddr3_cs_n),
        .ddr3_dm                 (ddr3_dm),
        .ddr3_odt                (ddr3_odt),
        .axi4_aw_slv             (ddr_axi4_aw),
        .axi4_w_slv              (ddr_axi4_w),
        .axi4_b_mst              (ddr_axi4_b),
        .axi4_ar_slv             (ddr_axi4_ar),
        .axi4_r_mst              (ddr_axi4_r)
    );

    soc u_soc(
        .clk                 (soc_clk),
        .rst_n               (soc_rst_n_gated),
        .uart_tx             (soc_uart_tx),
        .uart_rx             (uart_rx),
        .gpio_in             (gpio_in),
        .gpio_out            (gpio_out),
        .gpio_oe             (gpio_oe),
        .ddr_axi4_aw_mst     (ddr_axi4_aw),
        .ddr_axi4_w_mst      (ddr_axi4_w),
        .ddr_axi4_b_slv      (ddr_axi4_b),
        .ddr_axi4_ar_mst     (ddr_axi4_ar),
        .ddr_axi4_r_slv      (ddr_axi4_r),
        .flash_axi4_aw_mst   (flash_axi4_aw),
        .flash_axi4_w_mst    (flash_axi4_w),
        .flash_axi4_b_slv    (flash_axi4_b),
        .flash_axi4_ar_mst   (flash_axi4_ar),
        .flash_axi4_r_slv    (flash_axi4_r)
    );

    xilinx_axi_qspi #(
        .FLASH_AW (FLASH_AW)
    ) u_flash(
        .clk         (soc_clk),
        .rst_n       (soc_rst_n_gated),
        .axi4_aw_slv (flash_axi4_aw),
        .axi4_w_slv  (flash_axi4_w),
        .axi4_b_mst  (flash_axi4_b),
        .axi4_ar_slv (flash_axi4_ar),
        .axi4_r_mst  (flash_axi4_r),
        .flash_dq    (flash_dq),
        .flash_ce_n  (flash_ce_n)
    );

endmodule
