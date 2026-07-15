module uart_tx #(
    parameter BCW = 16
)(
    input  logic           clk,
    input  logic           rst_n,
    input  logic [BCW-1:0] bc,
    input  logic           ch_vld,
    input  logic [7:0]     ch,
    output logic           done,
    output logic           tx
    );
    logic [9:0] bit_sr;
    logic       se;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            bit_sr <= {10{1'b1}};
        else if (ch_vld)
            bit_sr <= { 1'b1, ch, 1'b0 };
        else if (se)
            bit_sr <= { 1'b1, bit_sr[9:1] };
    end

    logic [3:0] baud_cnt;
    logic       baud_cnt_inc;
    tri         baud_cnt_full = (baud_cnt == 4'd10);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            baud_cnt <= 4'd0;
        else if (baud_cnt_full)
            baud_cnt <= 4'd0;
        else if (baud_cnt_inc)
            baud_cnt <= baud_cnt + 1'b1;
    end

    logic [BCW-1:0] cycle_cnt;
    logic           cycle_cnt_inc;
    tri             cycle_cnt_full = (cycle_cnt == bc);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            cycle_cnt <= {BCW{1'b0}};
        else if (cycle_cnt_full)
            cycle_cnt <= {BCW{1'b0}};
        else if (cycle_cnt_inc)
            cycle_cnt <= cycle_cnt + 1'b1;
    end

    logic tx_pend;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            tx_pend <= 1'b0;
        else if (ch_vld)
            tx_pend <= 1'b1;
        else if (baud_cnt_full)
            tx_pend <= 1'b0;
    end

    assign se = cycle_cnt_full;
    assign baud_cnt_inc = se;
    assign cycle_cnt_inc = tx_pend;
    assign tx = bit_sr[0];
    assign done = baud_cnt_full;

endmodule

module uart_rx #(
    parameter BCW = 16
)(
    input  logic           clk,
    input  logic           rst_n,
    input  logic [BCW-1:0] bc,
    output logic           ch_vld,
    output logic [7:0]     ch,
    input  logic           rx
);
    logic [2:0] rx_sync;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            rx_sync <= 3'b111;
        else
            rx_sync <= { rx, rx_sync[2:1] };
    end

    logic [1:0] wind;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            wind <= 2'b11;
        else
            wind <= { wind[0], rx_sync[0] };
    end

    tri fall_edge = (wind == 2'b10);

    logic [9:0] bit_sr;
    logic       se;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            bit_sr <= {10{1'b1}};
        else if (se)
            bit_sr <= { rx_sync[0], bit_sr[9:1] };
    end

    logic [3:0] baud_cnt;
    logic       baud_cnt_inc;
    logic       ch_done;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            baud_cnt <= 4'd0;
        else if (ch_done)
            baud_cnt <= 4'd0;
        else if (baud_cnt_inc)
            baud_cnt <= baud_cnt + 1'b1;
    end

    logic idle_seen;
    logic [BCW-1:0] idle_cnt;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            idle_cnt <= {BCW{1'b0}};
            idle_seen <= 1'b0;
        end else if (rx_sync[0]) begin
            if (!idle_seen) begin
                if (idle_cnt == bc)
                    idle_seen <= 1'b1;
                else
                    idle_cnt <= idle_cnt + 1'b1;
            end
        end else begin
            idle_cnt <= {BCW{1'b0}};
        end
    end

    logic [BCW-1:0] cycle_cnt;
    logic           cycle_cnt_inc;
    tri             cycle_cnt_full = (cycle_cnt == bc);
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            cycle_cnt <= {BCW{1'b0}};
        else if (ch_done || baud_cnt_inc)
            cycle_cnt <= {BCW{1'b0}};
        else if (cycle_cnt_inc)
            cycle_cnt <= cycle_cnt + 1'b1;
    end

    logic rx_pend;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            rx_pend <= 1'b0;
        else if (!rx_pend && idle_seen && fall_edge)
            rx_pend <= 1'b1;
        else if (ch_done)
            rx_pend <= 1'b0;
    end

    logic ch_vld_q;
    logic [7:0] ch_q;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            ch_vld_q <= 1'b0;
            ch_q <= 8'b0;
        end else begin
            ch_vld_q <= ch_done;
            if (ch_done)
                ch_q <= bit_sr[9:2];
        end
    end

    assign se = rx_pend && (cycle_cnt == (bc >> 1));
    assign ch_done = se && (baud_cnt == 4'd9);
    assign baud_cnt_inc = rx_pend && cycle_cnt_full;
    assign cycle_cnt_inc = rx_pend;
    assign ch_vld = ch_vld_q;
    assign ch = ch_q;

endmodule

module apb_to_uart #(
    parameter UART_AW = 4,
    parameter UART_BCW = 16,
    parameter UART_RX_FIFO_DEPTH = 16
)(
    input  logic                clk,
    input  logic                rst_n,
    apb_req_if_t.slv            apb_req_slv,
    apb_rsp_if_t.mst            apb_rsp_mst,
    output logic [UART_BCW-1:0] bc,
    output logic                tx_ch_vld,
    output logic [7:0]          tx_ch,
    input  logic                tx_done,
    input  logic                rx_ch_vld,
    input  logic [7:0]          rx_ch,
    output logic                irq
);
    localparam REG_AW = UART_AW - 2;
    localparam logic [REG_AW-1:0] REG_BC_ADDR = 'd0;
    localparam logic [REG_AW-1:0] REG_TX_ADDR = 'd1;
    localparam logic [REG_AW-1:0] REG_RX_ADDR = 'd2;
    localparam logic [REG_AW-1:0] REG_STS_ADDR = 'd3;
    localparam logic [UART_BCW-1:0] UART_RESET_BC = UART_BCW'(9);

    logic reg_bc_set;
    logic [UART_BCW-1:0] reg_bc;
    logic [UART_BCW-1:0] reg_bc_nxt;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            reg_bc <= UART_RESET_BC;
        else if (reg_bc_set)
            reg_bc <= reg_bc_nxt;
    end

    logic apb_req_hsk;
    tri apb_req = apb_req_slv.psel & apb_req_slv.penable;
    tri [REG_AW-1:0] reg_addr = apb_req_slv.pkt.paddr[REG_AW+1:2];
    tri [31:0] reg_data = apb_req_slv.pkt.pwdata;
    tri apb_wr = apb_req_slv.pkt.pwrite & apb_req_hsk;

    localparam UART_RX_FIFO_PTR_W =
        (UART_RX_FIFO_DEPTH <= 1) ? 1 : $clog2(UART_RX_FIFO_DEPTH);
    localparam UART_RX_FIFO_CNT_W = $clog2(UART_RX_FIFO_DEPTH + 1);

    logic [7:0] rx_fifo[UART_RX_FIFO_DEPTH];
    logic [UART_RX_FIFO_PTR_W-1:0] rx_fifo_rd_ptr;
    logic [UART_RX_FIFO_PTR_W-1:0] rx_fifo_wr_ptr;
    logic [UART_RX_FIFO_CNT_W-1:0] rx_fifo_count;
    logic rx_fifo_push;
    logic rx_fifo_pop;
    logic rx_fifo_push_accept;
    tri rx_valid = rx_fifo_count != 0;
    tri rx_fifo_full = rx_fifo_count == UART_RX_FIFO_CNT_W'(UART_RX_FIFO_DEPTH);
    tri [7:0] rx_fifo_rdata = rx_valid ? rx_fifo[rx_fifo_rd_ptr] : 8'b0;

    assign rx_fifo_pop = apb_req_hsk && !apb_req_slv.pkt.pwrite &&
        reg_addr == REG_RX_ADDR && rx_valid;
    assign rx_fifo_push = rx_ch_vld;
    assign rx_fifo_push_accept = rx_fifo_push && (!rx_fifo_full || rx_fifo_pop);

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            rx_fifo_rd_ptr <= '0;
            rx_fifo_wr_ptr <= '0;
            rx_fifo_count <= '0;
        end else begin
            if (rx_fifo_push_accept) begin
                rx_fifo[rx_fifo_wr_ptr] <= rx_ch;
                if (rx_fifo_wr_ptr == UART_RX_FIFO_PTR_W'(UART_RX_FIFO_DEPTH - 1))
                    rx_fifo_wr_ptr <= '0;
                else
                    rx_fifo_wr_ptr <= rx_fifo_wr_ptr + 1'b1;
            end

            if (rx_fifo_pop) begin
                if (rx_fifo_rd_ptr == UART_RX_FIFO_PTR_W'(UART_RX_FIFO_DEPTH - 1))
                    rx_fifo_rd_ptr <= '0;
                else
                    rx_fifo_rd_ptr <= rx_fifo_rd_ptr + 1'b1;
            end

            case ({rx_fifo_push_accept, rx_fifo_pop})
                2'b10: rx_fifo_count <= rx_fifo_count + 1'b1;
                2'b01: rx_fifo_count <= rx_fifo_count - 1'b1;
                default: rx_fifo_count <= rx_fifo_count;
            endcase
        end
    end

    logic apb_req_write_pend;
    tri apb_rd_pend = !apb_req_write_pend;

    always_ff @(posedge clk) begin
        if (apb_req_hsk)
            apb_req_write_pend <= apb_req_slv.pkt.pwrite;
    end

    logic [31:0] reg_rdata;
    always_ff @(posedge clk) begin
        if (apb_req_hsk) begin
            case (reg_addr)
                REG_BC_ADDR: reg_rdata <= { {(32-UART_BCW){1'b0}}, reg_bc };
                REG_RX_ADDR: reg_rdata <= { 24'b0, rx_fifo_rdata };
                REG_STS_ADDR: reg_rdata <= { 31'b0, rx_valid };
                default: reg_rdata <= 32'b0;
            endcase
        end
    end

    localparam logic [1:0] S_IDLE    = 2'd0;
    localparam logic [1:0] S_REG_RW  = 2'd1;
    localparam logic [1:0] S_TX      = 2'd2;
    localparam logic [1:0] S_TX_DONE = 2'd3;

    logic [1:0] state;
    logic [1:0] state_nxt;

    tri at_s_idle = (state == S_IDLE);
    tri at_s_reg_rw = (state == S_REG_RW);
    tri at_s_tx = (state == S_TX);
    tri at_s_tx_done = (state == S_TX_DONE);

    assign apb_req_hsk = apb_req & at_s_idle;

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            state <= S_IDLE;
        else
            state <= state_nxt;
    end

    always_comb begin
        case (state)
            S_IDLE: begin
                if (apb_req)
                    if (reg_addr == REG_TX_ADDR &&
                        apb_req_slv.pkt.pwrite)
                        state_nxt = S_TX;
                    else
                        state_nxt = S_REG_RW;
                else
                    state_nxt = S_IDLE;
            end
            S_REG_RW: begin
                state_nxt = S_IDLE;
            end
            S_TX: begin
                if (tx_done)
                    state_nxt = S_TX_DONE;
                else
                    state_nxt = S_TX;
            end
            S_TX_DONE: begin
                state_nxt = S_IDLE;
            end
            default: state_nxt = 2'bxx;
        endcase
    end

    assign reg_bc_set = apb_wr && (reg_addr == REG_BC_ADDR);
    assign reg_bc_nxt = reg_data[UART_BCW-1:0];

    assign tx_ch_vld = apb_wr && (reg_addr == REG_TX_ADDR);
    assign tx_ch = reg_data[7:0];

    assign apb_rsp_mst.pready = at_s_reg_rw | at_s_tx_done;
    assign apb_rsp_mst.pkt.pslverr = 1'b0;
    assign apb_rsp_mst.pkt.prdata = apb_rd_pend ? reg_rdata : 32'b0;

    assign bc = reg_bc;
    assign irq = rx_valid;

endmodule

module apb_uart #(
    parameter UART_AW = 4,
    parameter UART_BCW = 16
)(
    input  logic        clk,
    input  logic        rst_n,
    apb_req_if_t.slv    apb_req_slv,
    apb_rsp_if_t.mst    apb_rsp_mst,
    input  logic        rx,
    output logic        tx,
    output logic        irq
);
    logic [UART_BCW-1:0] bc;
    logic                tx_ch_vld;
    logic [7:0]          tx_ch;
    logic                tx_done;
    logic                rx_ch_vld;
    logic [7:0]          rx_ch;

    apb_to_uart #(
        .UART_AW  (UART_AW),
        .UART_BCW (UART_BCW)
    ) u_apb_to_uart(
        .clk         (clk),
        .rst_n       (rst_n),
        .apb_req_slv (apb_req_slv),
        .apb_rsp_mst (apb_rsp_mst),
        .bc          (bc),
        .tx_ch_vld   (tx_ch_vld),
        .tx_ch       (tx_ch),
        .tx_done     (tx_done),
        .rx_ch_vld   (rx_ch_vld),
        .rx_ch       (rx_ch),
        .irq         (irq)
    );

    uart_tx #(
        .BCW         (UART_BCW)
    ) u_uart_tx(
        .clk         (clk),
        .rst_n       (rst_n),
        .bc          (bc),
        .ch_vld      (tx_ch_vld),
        .ch          (tx_ch),
        .done        (tx_done),
        .tx          (tx)
    );

    uart_rx #(
        .BCW         (UART_BCW)
    ) u_uart_rx(
        .clk         (clk),
        .rst_n       (rst_n),
        .bc          (bc),
        .ch_vld      (rx_ch_vld),
        .ch          (rx_ch),
        .rx          (rx)
    );

endmodule
