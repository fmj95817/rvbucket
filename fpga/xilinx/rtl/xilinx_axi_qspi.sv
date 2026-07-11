module xilinx_axi_qspi #(
    parameter FLASH_BASE = 32'h80000000,
    parameter FLASH_AW = 32,
    parameter SCK_HALF_PERIOD = 4
)(
    input logic      clk,
    input logic      rst_n,
    axi4_aw_if_t.slv axi4_aw_slv,
    axi4_w_if_t.slv  axi4_w_slv,
    axi4_b_if_t.mst  axi4_b_mst,
    axi4_ar_if_t.slv axi4_ar_slv,
    axi4_r_if_t.mst  axi4_r_mst,
    inout logic [3:0] flash_dq,
    output logic      flash_ce_n
);
    typedef enum logic [1:0] {
        RD_INIT,
        RD_IDLE,
        RD_SHIFT,
        RD_RESP
    } rd_state_t;

    typedef enum logic [1:0] {
        WR_IDLE,
        WR_WAIT,
        WR_RESP
    } wr_state_t;

    localparam int SCK_CNT_W =
        (SCK_HALF_PERIOD <= 1) ? 1 : $clog2(SCK_HALF_PERIOD);
    localparam logic [7:0] READ4_CMD = 8'h13;

    rd_state_t rd_state;
    wr_state_t wr_state;
    logic [7:0] rd_id;
    logic [31:0] rd_addr;
    logic [39:0] tx_shift;
    logic [5:0] tx_cnt;
    logic [5:0] rx_cnt;
    logic [31:0] rx_shift;
    logic [31:0] rd_data;
    logic [SCK_CNT_W-1:0] sck_cnt;
    logic spi_sck;
    logic spi_mosi;
    logic spi_miso;
    logic spi_active;
    logic rd_discard;
    logic aw_done;
    logic w_done;
    logic [7:0] wr_id;

    assign flash_ce_n = !spi_active;
    assign flash_dq[0] = spi_active ? spi_mosi : 1'bz;
    assign spi_miso = flash_dq[1];
    assign flash_dq[2] = 1'b1;
    assign flash_dq[3] = 1'b1;

    STARTUPE2 #(
        .PROG_USR      ("FALSE"),
        .SIM_CCLK_FREQ (0.0)
    ) u_startupe2(
        .CFGCLK    (),
        .CFGMCLK   (),
        .EOS       (),
        .PREQ      (),
        .CLK       (1'b0),
        .GSR       (1'b0),
        .GTS       (1'b0),
        .KEYCLEARB (1'b0),
        .PACK      (1'b0),
        .USRCCLKO  (spi_sck),
        .USRCCLKTS (!spi_active),
        .USRDONEO  (1'b1),
        .USRDONETS (1'b1)
    );

    wire sck_tick = sck_cnt == SCK_CNT_W'(SCK_HALF_PERIOD - 1);
    wire sck_rise = sck_tick && !spi_sck;
    wire sck_fall = sck_tick && spi_sck;
    wire [31:0] flash_addr = axi4_ar_slv.pkt.addr - FLASH_BASE;
    wire aw_hsk = axi4_aw_slv.vld && axi4_aw_slv.rdy;
    wire w_hsk = axi4_w_slv.vld && axi4_w_slv.rdy;
    wire b_hsk = axi4_b_mst.vld && axi4_b_mst.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_state <= RD_INIT;
            rd_id <= '0;
            rd_addr <= '0;
            tx_shift <= '0;
            tx_cnt <= '0;
            rx_cnt <= '0;
            rx_shift <= '0;
            rd_data <= '0;
            sck_cnt <= '0;
            spi_sck <= 1'b0;
            spi_mosi <= 1'b0;
            spi_active <= 1'b0;
            rd_discard <= 1'b0;
        end else begin
            case (rd_state)
            RD_INIT: begin
                sck_cnt <= '0;
                spi_sck <= 1'b0;
                spi_active <= 1'b1;
                spi_mosi <= READ4_CMD[7];
                tx_shift <= {READ4_CMD, 32'h0};
                tx_cnt <= '0;
                rx_cnt <= '0;
                rx_shift <= '0;
                rd_discard <= 1'b1;
                rd_state <= RD_SHIFT;
            end
            RD_IDLE: begin
                sck_cnt <= '0;
                spi_sck <= 1'b0;
                spi_active <= 1'b0;
                spi_mosi <= 1'b0;
                if (axi4_ar_slv.vld && axi4_ar_slv.rdy) begin
                    rd_id <= axi4_ar_slv.pkt.id;
                    rd_addr <= flash_addr;
                    tx_shift <= {READ4_CMD, flash_addr};
                    tx_cnt <= '0;
                    rx_cnt <= '0;
                    rx_shift <= '0;
                    spi_mosi <= READ4_CMD[7];
                    spi_active <= 1'b1;
                    rd_discard <= 1'b0;
                    rd_state <= RD_SHIFT;
                end
            end
            RD_SHIFT: begin
                if (sck_tick) begin
                    sck_cnt <= '0;
                    spi_sck <= !spi_sck;

                    if (sck_rise && tx_cnt >= 6'd40 && rx_cnt < 6'd32) begin
                        rx_shift <= {rx_shift[30:0], spi_miso};
                        rx_cnt <= rx_cnt + 1'b1;
                    end

                    if (sck_fall) begin
                        if (tx_cnt < 6'd39) begin
                            tx_cnt <= tx_cnt + 1'b1;
                            spi_mosi <= tx_shift[6'd38 - tx_cnt];
                        end else if (tx_cnt == 6'd39) begin
                            tx_cnt <= 6'd40;
                            spi_mosi <= 1'b0;
                        end else if (rx_cnt == 6'd32) begin
                            spi_active <= 1'b0;
                            spi_sck <= 1'b0;
                            if (rd_discard) begin
                                rd_discard <= 1'b0;
                                rd_state <= RD_IDLE;
                            end else begin
                                rd_data <= {
                                    rx_shift[7:0],
                                    rx_shift[15:8],
                                    rx_shift[23:16],
                                    rx_shift[31:24]
                                };
                                rd_state <= RD_RESP;
                            end
                        end
                    end
                end else begin
                    sck_cnt <= sck_cnt + 1'b1;
                end
            end
            RD_RESP: begin
                if (axi4_r_mst.vld && axi4_r_mst.rdy)
                    rd_state <= RD_IDLE;
            end
            default: begin
                rd_state <= RD_IDLE;
            end
            endcase
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_state <= WR_IDLE;
            aw_done <= 1'b0;
            w_done <= 1'b0;
            wr_id <= '0;
        end else begin
            case (wr_state)
            WR_IDLE: begin
                aw_done <= 1'b0;
                w_done <= 1'b0;
                if (aw_hsk) begin
                    aw_done <= 1'b1;
                    wr_id <= axi4_aw_slv.pkt.id;
                end
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_hsk || aw_done) && (w_hsk || w_done))
                    wr_state <= WR_RESP;
                else if (aw_hsk || w_hsk)
                    wr_state <= WR_WAIT;
            end
            WR_WAIT: begin
                if (aw_hsk) begin
                    aw_done <= 1'b1;
                    wr_id <= axi4_aw_slv.pkt.id;
                end
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_hsk || aw_done) && (w_hsk || w_done))
                    wr_state <= WR_RESP;
            end
            WR_RESP: begin
                if (b_hsk)
                    wr_state <= WR_IDLE;
            end
            default: begin
                wr_state <= WR_IDLE;
            end
            endcase
        end
    end

    assign axi4_ar_slv.rdy = rd_state == RD_IDLE;
    assign axi4_r_mst.vld = rd_state == RD_RESP;
    assign axi4_r_mst.pkt.id = rd_id;
    assign axi4_r_mst.pkt.data = rd_data;
    assign axi4_r_mst.pkt.resp = AXI4_R_RESP_OKAY;
    assign axi4_r_mst.pkt.last = 1'b1;

    assign axi4_aw_slv.rdy = wr_state == WR_IDLE || wr_state == WR_WAIT;
    assign axi4_w_slv.rdy = wr_state == WR_IDLE || wr_state == WR_WAIT;
    assign axi4_b_mst.vld = wr_state == WR_RESP;
    assign axi4_b_mst.pkt.id = wr_id;
    assign axi4_b_mst.pkt.resp = AXI4_B_RESP_OKAY;

endmodule
