`include "bti.svh"

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
            wind <= { rx_sync[0], wind[1] };
    end

    tri fall_edge = (wind == 2'b01);

    logic [9:0] bit_sr;
    logic       se;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            bit_sr <= {10{1'b1}};
        else if (se)
            bit_sr <= { wind[0], bit_sr[9:1] };
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

    logic rx_pend;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            rx_pend <= 1'b0;
        else if (fall_edge)
            rx_pend <= 1'b1;
        else if (baud_cnt_full)
            rx_pend <= 1'b0;
    end

    assign se = (cycle_cnt == {1'b0, bc[BCW-2:0]});
    assign baud_cnt_inc = cycle_cnt_full;
    assign cycle_cnt_inc = rx_pend;
    assign ch_vld = baud_cnt_full;
    assign ch = bit_sr[8:1];

endmodule

module bti_to_uart #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter UART_AW = 4,
    parameter UART_BCW = 16
)(
    input  logic                clk,
    input  logic                rst_n,
    bti_req_if_t.slv            bti_req_slv,
    bti_rsp_if_t.mst            bti_rsp_mst,
    output logic [UART_BCW-1:0] bc,
    output logic                tx_ch_vld,
    output logic [7:0]          tx_ch,
    input  logic                tx_done,
    input  logic                rx_ch_vld,
    input  logic [7:0]          rx_ch
);
    localparam REG_AW = UART_AW - 2;
    localparam logic [REG_AW-1:0] REG_BC_ADDR = 'd0;
    localparam logic [REG_AW-1:0] REG_TX_ADDR = 'd1;
    localparam logic [REG_AW-1:0] REG_RX_ADDR = 'd2;

    logic reg_bc_set;
    logic [UART_BCW-1:0] reg_bc;
    logic [UART_BCW-1:0] reg_bc_nxt;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            reg_bc <= {UART_BCW{1'b1}};
        else if (reg_bc_set)
            reg_bc <= reg_bc_nxt;
    end

    logic reg_rx_set;
    logic [7:0] reg_rx;
    logic [7:0] reg_rx_nxt;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            reg_rx <= 8'd0;
        else if (reg_rx_set)
            reg_rx <= reg_rx_nxt;
    end

    tri bti_req_hsk = bti_req_slv.vld & bti_req_slv.rdy;
    tri bti_rsp_hsk = bti_rsp_mst.vld & bti_rsp_mst.rdy;

    tri [REG_AW-1:0] reg_addr = bti_req_slv.pkt.addr[REG_AW+1:2];
    tri [BTI_DW-1:0] reg_data = bti_req_slv.pkt.data;
    tri bti_req_cmd = bti_req_slv.pkt.cmd;
    tri bti_wr = (bti_req_cmd == BTI_CMD_WRITE) && bti_req_hsk;

    logic bti_req_cmd_pend;
    tri bti_rd_pend = (bti_req_cmd_pend == BTI_CMD_READ);

    always_ff @(posedge clk) begin
        if (bti_req_hsk)
            bti_req_cmd_pend <= bti_req_slv.pkt.cmd;
    end

    logic [BTI_DW-1:0] reg_rdata;
    always_ff @(posedge clk) begin
        if (bti_req_hsk) begin
            case (reg_addr)
                REG_BC_ADDR: reg_rdata <= { {(BTI_DW-UART_BCW){1'b0}}, reg_bc };
                REG_RX_ADDR: reg_rdata <= { {(BTI_DW-8){1'b0}}, reg_rx };
                default: reg_rdata <= {BTI_DW{1'b0}};
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

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            state <= S_IDLE;
        else
            state <= state_nxt;
    end

    always_comb begin
        case (state)
            S_IDLE: begin
                if (bti_req_slv.vld)
                    if (reg_addr == REG_TX_ADDR &&
                        bti_req_cmd == BTI_CMD_WRITE)
                        state_nxt = S_TX;
                    else
                        state_nxt = S_REG_RW;
                else
                    state_nxt = S_IDLE;
            end
            S_REG_RW: begin
                if (bti_rsp_mst.rdy)
                    state_nxt = S_IDLE;
                else
                    state_nxt = S_REG_RW;
            end
            S_TX: begin
                if (tx_done)
                    state_nxt = S_TX_DONE;
                else
                    state_nxt = S_TX;
            end
            S_TX_DONE: begin
                if (bti_rsp_mst.rdy)
                    state_nxt = S_IDLE;
                else
                    state_nxt = S_TX_DONE;
            end
            default: state_nxt = 2'bxx;
        endcase
    end

    assign reg_bc_set = bti_wr && (reg_addr == REG_BC_ADDR);
    assign reg_bc_nxt = reg_data[UART_BCW-1:0];

    assign reg_rx_set = rx_ch_vld;
    assign reg_rx_nxt = rx_ch;

    assign tx_ch_vld = bti_wr && (reg_addr == REG_TX_ADDR);
    assign tx_ch = reg_data[7:0];

    assign bti_req_slv.rdy = at_s_idle;
    assign bti_rsp_mst.vld = at_s_reg_rw | at_s_tx_done;
    assign bti_rsp_mst.pkt.ok = 1'b1;
    assign bti_rsp_mst.pkt.data = bti_rd_pend ? reg_rdata : {BTI_DW{1'b0}};

    assign bc = reg_bc;

endmodule

module bti_uart #(
    parameter BTI_AW = 32,
    parameter BTI_DW = 32,
    parameter UART_AW = 4,
    parameter UART_BCW = 16
)(
    input  logic        clk,
    input  logic        rst_n,
    bti_req_if_t.slv    bti_req_slv,
    bti_rsp_if_t.mst    bti_rsp_mst,
    input  logic        rx,
    output logic        tx
);
    logic [UART_BCW-1:0] bc;
    logic                tx_ch_vld;
    logic [7:0]          tx_ch;
    logic                tx_done;
    logic                rx_ch_vld;
    logic [7:0]          rx_ch;

    bti_to_uart #(
        .BTI_AW      (BTI_AW),
        .BTI_DW      (BTI_DW),
        .UART_AW     (UART_AW)
    ) u_bti_to_uart(
        .clk         (clk),
        .rst_n       (rst_n),
        .bti_req_slv (bti_req_slv),
        .bti_rsp_mst (bti_rsp_mst),
        .bc          (bc),
        .tx_ch_vld   (tx_ch_vld),
        .tx_ch       (tx_ch),
        .tx_done     (tx_done),
        .rx_ch_vld   (rx_ch_vld),
        .rx_ch       (rx_ch)
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
