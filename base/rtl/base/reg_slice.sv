module reg_slice #(
    parameter DW = 1
)(
    input           clk,
    input           rst_n,

    input           src_vld,
    output          src_rdy,
    input  [DW-1:0] src_data,

    output          dst_vld,
    input           dst_rdy,
    output [DW-1:0] dst_data
);

    logic data_vld;
    logic [DW-1:0] data;

    tri data_vld_set = src_vld & src_rdy;
    tri data_vld_clear = dst_vld & dst_rdy;
    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            data_vld <= 1'b0;
        else if (data_vld_set)
            data_vld <= 1'b1;
        else if (data_vld_clear)
            data_vld <= 1'b0;
    end

    always_ff @(posedge clk) begin
        if (data_vld_set)
            data <= src_data;
    end

    assign dst_vld = data_vld;
    assign src_rdy = (~data_vld) | data_vld_clear;
    assign dst_data = data;

endmodule