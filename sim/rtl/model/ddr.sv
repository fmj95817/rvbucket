`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi_ddr #(
    parameter DDR_AW = 28,
    parameter RD_LATENCY = 0,
    parameter WR_LATENCY = 0
)(
    input logic      clk,
    input logic      rst_n,
    axi4_aw_if_t.slv axi4_aw_slv,
    axi4_w_if_t.slv  axi4_w_slv,
    axi4_b_if_t.mst  axi4_b_mst,
    axi4_ar_if_t.slv axi4_ar_slv,
    axi4_r_if_t.mst  axi4_r_mst
);
    localparam WORD_AW = DDR_AW - 2;
    localparam int unsigned WORD_NUM = 32'd1 << WORD_AW;

    logic [31:0] mem[0:WORD_NUM-1];

    logic rd_pending;
    logic [31:0] rd_delay;
    logic r_vld;
    logic [7:0] r_id;
    logic [31:0] r_addr;
    logic [7:0] r_len;
    logic [2:0] r_size;
    logic [1:0] r_burst;
    logic [7:0] r_beat;
    logic [31:0] r_data;

    logic wr_pending;
    logic wr_rsp_pending;
    logic [31:0] wr_delay;
    logic b_vld;
    logic [7:0] b_id;
    logic [31:0] w_addr;
    logic [7:0] w_len;
    logic [2:0] w_size;
    logic [1:0] w_burst;
    logic [7:0] w_beat;

    wire rd_req_hsk = axi4_ar_slv.vld && axi4_ar_slv.rdy;
    wire rd_rsp_hsk = axi4_r_mst.vld && axi4_r_mst.rdy;
    wire aw_hsk = axi4_aw_slv.vld && axi4_aw_slv.rdy;
    wire w_hsk = axi4_w_slv.vld && axi4_w_slv.rdy;
    wire wr_rsp_hsk = axi4_b_mst.vld && axi4_b_mst.rdy;

    function automatic logic [31:0] next_addr(
        input logic [31:0] addr,
        input logic [2:0] size,
        input logic [1:0] burst
    );
        unique case (burst)
        2'd0: next_addr = addr;
        default: next_addr = addr + (32'd1 << size);
        endcase
    endfunction

    function automatic logic [WORD_AW-1:0] word_addr(
        input logic [31:0] byte_addr
    );
        word_addr = byte_addr[WORD_AW+1:2];
    endfunction

    function automatic logic [7:0] read_byte(
        input logic [31:0] byte_addr
    );
        unique case (byte_addr[1:0])
        2'd0: read_byte = mem[word_addr(byte_addr)][7:0];
        2'd1: read_byte = mem[word_addr(byte_addr)][15:8];
        2'd2: read_byte = mem[word_addr(byte_addr)][23:16];
        2'd3: read_byte = mem[word_addr(byte_addr)][31:24];
        default: read_byte = 8'hxx;
        endcase
    endfunction

    function automatic logic [31:0] read_word(
        input logic [31:0] byte_addr
    );
        read_word = {
            read_byte(byte_addr + 32'd3),
            read_byte(byte_addr + 32'd2),
            read_byte(byte_addr + 32'd1),
            read_byte(byte_addr)
        };
    endfunction

    task automatic write_byte(
        input logic [31:0] byte_addr,
        input logic [7:0] data
    );
        unique case (byte_addr[1:0])
        2'd0: mem[word_addr(byte_addr)][7:0] = data;
        2'd1: mem[word_addr(byte_addr)][15:8] = data;
        2'd2: mem[word_addr(byte_addr)][23:16] = data;
        2'd3: mem[word_addr(byte_addr)][31:24] = data;
        default: begin end
        endcase
    endtask

    task automatic write_word(
        input logic [31:0] byte_addr,
        input logic [31:0] data,
        input logic [3:0] strb
    );
        for (int i = 0; i < 4; i++) begin
            if (strb[i])
                write_byte(byte_addr + i[31:0], data[i*8 +: 8]);
        end
    endtask

    initial begin
        for (int unsigned i = 0; i < WORD_NUM; i++)
            mem[i] = '0;
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_pending <= 1'b0;
            rd_delay <= '0;
            r_vld <= 1'b0;
            r_id <= '0;
            r_addr <= '0;
            r_len <= '0;
            r_size <= '0;
            r_burst <= '0;
            r_beat <= '0;
            r_data <= '0;
        end else begin
            if (rd_rsp_hsk) begin
                r_vld <= 1'b0;
                if (r_beat == r_len) begin
                    rd_pending <= 1'b0;
                end else begin
                    r_beat <= r_beat + 1'b1;
                    r_addr <= next_addr(r_addr, r_size, r_burst);
                    rd_delay <= RD_LATENCY;
                end
            end

            if (!rd_pending && rd_req_hsk) begin
                rd_pending <= 1'b1;
                rd_delay <= RD_LATENCY;
                r_id <= axi4_ar_slv.pkt.id;
                r_addr <= axi4_ar_slv.pkt.addr;
                r_len <= axi4_ar_slv.pkt.len;
                r_size <= axi4_ar_slv.pkt.size;
                r_burst <= axi4_ar_slv.pkt.burst;
                r_beat <= '0;
            end else if (rd_pending && !r_vld) begin
                if (rd_delay == 0) begin
                    r_data <= read_word(r_addr);
                    r_vld <= 1'b1;
                end else begin
                    rd_delay <= rd_delay - 1'b1;
                end
            end
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_pending <= 1'b0;
            wr_rsp_pending <= 1'b0;
            wr_delay <= '0;
            b_vld <= 1'b0;
            b_id <= '0;
            w_addr <= '0;
            w_len <= '0;
            w_size <= '0;
            w_burst <= '0;
            w_beat <= '0;
        end else begin
            if (wr_rsp_hsk) begin
                b_vld <= 1'b0;
                wr_rsp_pending <= 1'b0;
            end

            if (!wr_pending && !wr_rsp_pending && aw_hsk) begin
                wr_pending <= 1'b1;
                b_id <= axi4_aw_slv.pkt.id;
                w_addr <= axi4_aw_slv.pkt.addr;
                w_len <= axi4_aw_slv.pkt.len;
                w_size <= axi4_aw_slv.pkt.size;
                w_burst <= axi4_aw_slv.pkt.burst;
                w_beat <= '0;
            end

            if (wr_pending && w_hsk) begin
                write_word(w_addr, axi4_w_slv.pkt.data, axi4_w_slv.pkt.strb);
                if (axi4_w_slv.pkt.last || w_beat == w_len) begin
                    wr_pending <= 1'b0;
                    wr_rsp_pending <= 1'b1;
                    wr_delay <= WR_LATENCY;
                end else begin
                    w_beat <= w_beat + 1'b1;
                    w_addr <= next_addr(w_addr, w_size, w_burst);
                end
            end else if (wr_rsp_pending && !b_vld) begin
                if (wr_delay == 0) begin
                    b_vld <= 1'b1;
                end else begin
                    wr_delay <= wr_delay - 1'b1;
                end
            end
        end
    end

    assign axi4_ar_slv.rdy = !rd_pending;

    assign axi4_r_mst.vld = r_vld;
    assign axi4_r_mst.pkt.id = r_id;
    assign axi4_r_mst.pkt.data = r_data;
    assign axi4_r_mst.pkt.resp = AXI4_R_RESP_OKAY;
    assign axi4_r_mst.pkt.last = r_beat == r_len;

    assign axi4_aw_slv.rdy = !wr_pending && !wr_rsp_pending && !b_vld;
    assign axi4_w_slv.rdy = wr_pending;

    assign axi4_b_mst.vld = b_vld;
    assign axi4_b_mst.pkt.id = b_id;
    assign axi4_b_mst.pkt.resp = AXI4_B_RESP_OKAY;
endmodule
