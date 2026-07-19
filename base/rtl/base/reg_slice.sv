module vld_reg_slice #(
    parameter int DW = 1
)(
    input logic          clk,
    input logic          rst_n,
    input logic          clear,

    input logic          src_vld,
    output logic         src_rdy,
    input logic [DW-1:0] src_data,

    output logic          dst_vld,
    input logic           dst_rdy,
    output logic [DW-1:0] dst_data
);
    logic data_vld;
    logic [DW-1:0] data;

    wire data_vld_set = src_vld && src_rdy;
    wire data_vld_clear = dst_vld && dst_rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            data_vld <= 1'b0;
        else if (clear)
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
    assign src_rdy = !data_vld || data_vld_clear;
    assign dst_data = data;
endmodule

module rdy_reg_slice #(
    parameter int DW = 1
)(
    input logic          clk,
    input logic          rst_n,
    input logic          clear,

    input logic          src_vld,
    output logic         src_rdy,
    input logic [DW-1:0] src_data,

    output logic          dst_vld,
    input logic           dst_rdy,
    output logic [DW-1:0] dst_data
);
    logic [DW-1:0] data;
    logic data_en;
    logic full_en;
    logic full;

    assign data_en = src_vld && !full && !dst_rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            data <= '0;
        else if (data_en)
            data <= src_data;
    end

    assign full_en = data_en || dst_rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            full <= 1'b0;
        else if (clear)
            full <= 1'b0;
        else if (full_en)
            full <= data_en;
    end

    assign src_rdy = !full;
    assign dst_vld = src_vld || full;
    assign dst_data = full ? data : src_data;
endmodule

module bidir_reg_slice #(
    parameter int DW = 1
)(
    input logic          clk,
    input logic          rst_n,
    input logic          clear,

    input logic          src_vld,
    output logic         src_rdy,
    input logic [DW-1:0] src_data,

    output logic          dst_vld,
    input logic           dst_rdy,
    output logic [DW-1:0] dst_data
);
    logic mid_vld;
    logic mid_rdy;
    logic [DW-1:0] mid_data;

    rdy_reg_slice #(
        .DW (DW)
    ) u_rdy_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (clear),
        .src_vld  (src_vld),
        .src_rdy  (src_rdy),
        .src_data (src_data),
        .dst_vld  (mid_vld),
        .dst_rdy  (mid_rdy),
        .dst_data (mid_data)
    );

    vld_reg_slice #(
        .DW (DW)
    ) u_vld_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (clear),
        .src_vld  (mid_vld),
        .src_rdy  (mid_rdy),
        .src_data (mid_data),
        .dst_vld  (dst_vld),
        .dst_rdy  (dst_rdy),
        .dst_data (dst_data)
    );
endmodule
