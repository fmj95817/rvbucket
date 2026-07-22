module fifo #(
    parameter int DW = 1,
    parameter int DEPTH = 2,
    parameter bit FALL_THROUGH = 1'b0,
    localparam int PTR_W = $clog2(DEPTH)
)(
    input logic              clk,
    input logic              rst_n,
    input logic              clear,

    input logic              wr_vld,
    output logic             wr_rdy,
    input logic [DW-1:0]     wr_data,

    output logic             rd_vld,
    input logic              rd_rdy,
    output logic [DW-1:0]    rd_data,

    output logic             empty,
    output logic             full
);
    logic [DW-1:0] mem[DEPTH];
    logic [PTR_W:0] rptr;
    logic [PTR_W:0] wptr;
    logic [PTR_W-1:0] widx;

    wire ptr_msb_diff = rptr[PTR_W] != wptr[PTR_W];
    wire ptr_lsb_same = rptr[PTR_W-1:0] == wptr[PTR_W-1:0];
    wire wr_hsk;
    wire rd_hsk;

`ifndef SYNTHESIS
    initial begin
        assert (DEPTH >= 2)
            else $fatal(1, "fifo DEPTH must be >= 2");
        assert ((DEPTH & (DEPTH - 1)) == 0)
            else $fatal(1, "fifo DEPTH must be power-of-two");
    end
`endif

    assign empty = rptr == wptr;
    assign full = ptr_msb_diff & ptr_lsb_same;
    assign rd_vld = !empty || (FALL_THROUGH && wr_vld);
    assign wr_rdy = !full;
    assign wr_hsk = wr_vld & wr_rdy;
    assign rd_hsk = rd_vld & rd_rdy;
    assign widx = wptr[PTR_W-1:0];
    assign rd_data = !empty ? mem[rptr[PTR_W-1:0]] :
        (FALL_THROUGH ? wr_data : {DW{1'b0}});

    always_ff @(posedge clk) begin
        if (wr_hsk)
            mem[widx] <= wr_data;
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rptr <= '0;
            wptr <= '0;
        end else if (clear) begin
            rptr <= '0;
            wptr <= '0;
        end else begin
            if (wr_hsk)
                wptr <= wptr + 1'b1;
            if (rd_hsk)
                rptr <= rptr + 1'b1;
        end
    end
endmodule
