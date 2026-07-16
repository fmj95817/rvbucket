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
    logic [1:0] entry_count;
    logic [DW-1:0] entry0;
    logic [DW-1:0] entry1;

    wire src_hsk = src_vld && src_rdy;
    wire dst_hsk = dst_vld && dst_rdy;
    wire bypass_hsk = entry_count == 2'd0 && src_hsk && dst_hsk;

    assign src_rdy = entry_count != 2'd2;
    assign dst_vld = entry_count != 2'd0 || src_vld;
    assign dst_data = entry_count != 2'd0 ? entry0 : src_data;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            entry_count <= 2'd0;
        end else if (clear) begin
            entry_count <= 2'd0;
        end else if (!bypass_hsk) begin
            unique case ({src_hsk, dst_hsk})
            2'b10: begin
                if (entry_count == 2'd0)
                    entry0 <= src_data;
                else
                    entry1 <= src_data;
                entry_count <= entry_count + 1'b1;
            end
            2'b01: begin
                if (entry_count == 2'd2)
                    entry0 <= entry1;
                entry_count <= entry_count - 1'b1;
            end
            2'b11: begin
                if (entry_count == 2'd1)
                    entry0 <= src_data;
            end
            default: begin
            end
            endcase
        end
    end
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
