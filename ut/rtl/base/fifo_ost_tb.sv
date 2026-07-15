module fifo_ost_tb;
    logic clk;
    logic rst_n;

    logic fifo_clear;
    logic fifo_wr_vld;
    logic fifo_wr_rdy;
    logic [7:0] fifo_wr_data;
    logic fifo_rd_vld;
    logic fifo_rd_rdy;
    logic [7:0] fifo_rd_data;
    logic fifo_empty;
    logic fifo_full;

    logic ostq_alloc_vld;
    logic ostq_alloc_rdy;
    logic [7:0] ostq_alloc_ctx;
    logic [1:0] ostq_alloc_slot;
    logic ostq_head_vld;
    logic [7:0] ostq_head_ctx;
    logic [1:0] ostq_head_slot;
    logic ostq_free_head;
    logic ostq_slot_wr_vld;
    logic [1:0] ostq_slot_wr_idx;
    logic [7:0] ostq_slot_wr_ctx;
    logic [3:0] ostq_slot_vld;
    logic [31:0] ostq_slot_ctx_flat;
    logic ostq_empty;
    logic ostq_full;

    logic ostk_alloc_vld;
    logic ostk_alloc_rdy;
    logic [1:0] ostk_alloc_key;
    logic [7:0] ostk_alloc_ctx;
    logic [1:0] ostk_alloc_slot;
    logic [1:0] ostk_lookup_key;
    logic ostk_lookup_vld;
    logic [1:0] ostk_lookup_slot;
    logic [7:0] ostk_lookup_ctx;
    logic ostk_free_vld;
    logic [1:0] ostk_free_slot;
    logic ostk_empty;
    logic ostk_full;

    fifo #(
        .DW(8),
        .DEPTH(4)
    ) u_fifo (
        .clk(clk),
        .rst_n(rst_n),
        .clear(fifo_clear),
        .wr_vld(fifo_wr_vld),
        .wr_rdy(fifo_wr_rdy),
        .wr_data(fifo_wr_data),
        .rd_vld(fifo_rd_vld),
        .rd_rdy(fifo_rd_rdy),
        .rd_data(fifo_rd_data),
        .empty(fifo_empty),
        .full(fifo_full)
    );

    ostq #(
        .DW(8),
        .DEPTH(4)
    ) u_ostq (
        .clk(clk),
        .rst_n(rst_n),
        .alloc_vld(ostq_alloc_vld),
        .alloc_rdy(ostq_alloc_rdy),
        .alloc_ctx(ostq_alloc_ctx),
        .alloc_slot(ostq_alloc_slot),
        .head_vld(ostq_head_vld),
        .head_ctx(ostq_head_ctx),
        .head_slot(ostq_head_slot),
        .free_head(ostq_free_head),
        .slot_wr_vld(ostq_slot_wr_vld),
        .slot_wr_idx(ostq_slot_wr_idx),
        .slot_wr_ctx(ostq_slot_wr_ctx),
        .slot_vld(ostq_slot_vld),
        .slot_ctx_flat(ostq_slot_ctx_flat),
        .empty(ostq_empty),
        .full(ostq_full)
    );

    ostk #(
        .KEY_W(2),
        .DW(8),
        .DEPTH(4)
    ) u_ostk (
        .clk(clk),
        .rst_n(rst_n),
        .alloc_vld(ostk_alloc_vld),
        .alloc_rdy(ostk_alloc_rdy),
        .alloc_key(ostk_alloc_key),
        .alloc_ctx(ostk_alloc_ctx),
        .alloc_slot(ostk_alloc_slot),
        .lookup_key(ostk_lookup_key),
        .lookup_vld(ostk_lookup_vld),
        .lookup_slot(ostk_lookup_slot),
        .lookup_ctx(ostk_lookup_ctx),
        .free_vld(ostk_free_vld),
        .free_slot(ostk_free_slot),
        .slot_wr_vld(1'b0),
        .slot_wr_idx('0),
        .slot_wr_ctx('0),
        .slot_vld(),
        .slot_key_flat(),
        .slot_ctx_flat(),
        .slot_seq_flat(),
        .empty(ostk_empty),
        .full(ostk_full)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    task automatic tick;
        begin
            @(posedge clk);
            #1;
        end
    endtask

    task automatic check(input logic cond, input string msg);
        begin
            if (!cond) begin
                $display("FAIL: %s", msg);
                $fatal(1);
            end
        end
    endtask

    task automatic reset_all;
        begin
            rst_n = 1'b0;
            fifo_clear = 1'b0;
            fifo_wr_vld = 1'b0;
            fifo_wr_data = '0;
            fifo_rd_rdy = 1'b0;
            ostq_alloc_vld = 1'b0;
            ostq_alloc_ctx = '0;
            ostq_free_head = 1'b0;
            ostq_slot_wr_vld = 1'b0;
            ostq_slot_wr_idx = '0;
            ostq_slot_wr_ctx = '0;
            ostk_alloc_vld = 1'b0;
            ostk_alloc_key = '0;
            ostk_alloc_ctx = '0;
            ostk_lookup_key = '0;
            ostk_free_vld = 1'b0;
            ostk_free_slot = '0;
            repeat (3) tick();
            rst_n = 1'b1;
            tick();
        end
    endtask

    task automatic fifo_push(input logic [7:0] data);
        begin
            fifo_wr_vld = 1'b1;
            fifo_wr_data = data;
            check(fifo_wr_rdy, "fifo push backpressured unexpectedly");
            tick();
            fifo_wr_vld = 1'b0;
            fifo_wr_data = '0;
        end
    endtask

    task automatic fifo_pop(input logic [7:0] exp);
        begin
            check(fifo_rd_vld, "fifo pop without data");
            check(fifo_rd_data == exp, "fifo data mismatch");
            fifo_rd_rdy = 1'b1;
            tick();
            fifo_rd_rdy = 1'b0;
        end
    endtask

    task automatic ostq_alloc(input logic [7:0] ctx);
        begin
            ostq_alloc_vld = 1'b1;
            ostq_alloc_ctx = ctx;
            check(ostq_alloc_rdy, "ostq alloc backpressured unexpectedly");
            tick();
            ostq_alloc_vld = 1'b0;
            ostq_alloc_ctx = '0;
        end
    endtask

    task automatic ostq_free(input logic [7:0] exp);
        begin
            check(ostq_head_vld, "ostq head invalid");
            check(ostq_head_ctx == exp, "ostq head mismatch");
            ostq_free_head = 1'b1;
            tick();
            ostq_free_head = 1'b0;
        end
    endtask

    task automatic ostk_alloc(input logic [1:0] key, input logic [7:0] ctx);
        begin
            ostk_alloc_vld = 1'b1;
            ostk_alloc_key = key;
            ostk_alloc_ctx = ctx;
            check(ostk_alloc_rdy, "ostk alloc backpressured unexpectedly");
            tick();
            ostk_alloc_vld = 1'b0;
            ostk_alloc_key = '0;
            ostk_alloc_ctx = '0;
        end
    endtask

    initial begin
        reset_all();

        check(fifo_empty, "fifo reset empty");
        fifo_push(8'h11);
        fifo_push(8'h22);
        fifo_push(8'h33);
        fifo_push(8'h44);
        check(fifo_full, "fifo full after four pushes");
        fifo_wr_vld = 1'b1;
        fifo_wr_data = 8'h55;
        check(!fifo_wr_rdy, "fifo rejects write when full");
        fifo_rd_rdy = 1'b1;
        #1;
        check(!fifo_wr_rdy, "fifo keeps write blocked on full+pop");
        tick();
        fifo_rd_rdy = 1'b0;
        #1;
        check(fifo_wr_rdy, "fifo accepts write after pop creates space");
        tick();
        fifo_wr_vld = 1'b0;
        fifo_pop(8'h22);
        fifo_pop(8'h33);
        fifo_pop(8'h44);
        fifo_pop(8'h55);
        check(fifo_empty, "fifo empty after pops");

        ostq_alloc(8'ha1);
        ostq_alloc(8'ha2);
        check(ostq_slot_vld[0] && ostq_slot_ctx_flat[0 +: 8] == 8'ha1,
            "ostq slot0 visible");
        ostq_slot_wr_vld = 1'b1;
        ostq_slot_wr_idx = 2'd0;
        ostq_slot_wr_ctx = 8'ha3;
        tick();
        ostq_slot_wr_vld = 1'b0;
        check(ostq_head_ctx == 8'ha3, "ostq slot writeback");
        ostq_free(8'ha3);
        ostq_free(8'ha2);
        check(ostq_empty, "ostq empty after frees");

        ostk_alloc(2'd1, 8'hb1);
        ostk_alloc(2'd2, 8'hc1);
        ostk_alloc(2'd1, 8'hb2);
        ostk_lookup_key = 2'd2;
        #1;
        check(ostk_lookup_vld && ostk_lookup_ctx == 8'hc1, "ostk key2 lookup");
        ostk_free_slot = ostk_lookup_slot;
        ostk_free_vld = 1'b1;
        tick();
        ostk_free_vld = 1'b0;
        ostk_lookup_key = 2'd1;
        #1;
        check(ostk_lookup_vld && ostk_lookup_ctx == 8'hb1, "ostk key1 first lookup");
        ostk_free_slot = ostk_lookup_slot;
        ostk_free_vld = 1'b1;
        tick();
        ostk_free_vld = 1'b0;
        #1;
        check(ostk_lookup_vld && ostk_lookup_ctx == 8'hb2, "ostk key1 second lookup");
        ostk_free_slot = ostk_lookup_slot;
        ostk_free_vld = 1'b1;
        tick();
        ostk_free_vld = 1'b0;
        tick();
        check(ostk_empty, "ostk empty after frees");

        $display("PASS: fifo_ost_tb");
        $finish;
    end
endmodule
