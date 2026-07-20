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

    logic ft_wr_vld;
    logic ft_wr_rdy;
    logic [7:0] ft_wr_data;
    logic ft_rd_vld;
    logic ft_rd_rdy;
    logic [7:0] ft_rd_data;
    logic ft_empty;

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

    logic slice_src_vld;
    logic slice_src_rdy;
    logic [7:0] slice_src_data;
    logic slice_clear;
    logic slice_dst_vld;
    logic slice_dst_rdy;
    logic [7:0] slice_dst_data;

    logic [3:0] oldest_candidate_vld;
    logic [15:0] oldest_older_flat;
    logic oldest_select_vld;
    logic [1:0] oldest_select_slot;

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

    fifo #(
        .DW           (8),
        .DEPTH        (4),
        .FALL_THROUGH (1'b1)
    ) u_ft_fifo (
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (ft_wr_vld),
        .wr_rdy  (ft_wr_rdy),
        .wr_data (ft_wr_data),
        .rd_vld  (ft_rd_vld),
        .rd_rdy  (ft_rd_rdy),
        .rd_data (ft_rd_data),
        .empty   (ft_empty),
        .full    ()
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
        .slot_older_flat(),
        .empty(ostk_empty),
        .full(ostk_full)
    );

    bidir_reg_slice #(
        .DW (8)
    ) u_bidir_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (slice_clear),
        .src_vld  (slice_src_vld),
        .src_rdy  (slice_src_rdy),
        .src_data (slice_src_data),
        .dst_vld  (slice_dst_vld),
        .dst_rdy  (slice_dst_rdy),
        .dst_data (slice_dst_data)
    );

    oldest_select #(
        .DEPTH (4)
    ) u_oldest_select(
        .candidate_vld (oldest_candidate_vld),
        .older_flat    (oldest_older_flat),
        .select_vld    (oldest_select_vld),
        .select_slot   (oldest_select_slot)
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
            ft_wr_vld = 1'b0;
            ft_wr_data = '0;
            ft_rd_rdy = 1'b0;
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
            slice_src_vld = 1'b0;
            slice_src_data = '0;
            slice_clear = 1'b0;
            slice_dst_rdy = 1'b0;
            oldest_candidate_vld = '0;
            oldest_older_flat = '0;
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

        ft_wr_vld = 1'b1;
        ft_wr_data = 8'h61;
        ft_rd_rdy = 1'b1;
        #1;
        check(ft_wr_rdy && ft_rd_vld && ft_rd_data == 8'h61,
            "fall-through fifo direct transfer");
        tick();
        ft_wr_vld = 1'b0;
        ft_rd_rdy = 1'b0;
        #1;
        check(ft_empty, "fall-through fifo direct transfer stays empty");

        ft_wr_vld = 1'b1;
        ft_wr_data = 8'h62;
        #1;
        check(ft_rd_vld && ft_rd_data == 8'h62,
            "fall-through fifo exposes stalled input");
        tick();
        ft_wr_vld = 1'b0;
        ft_wr_data = '0;
        #1;
        check(!ft_empty && ft_rd_vld && ft_rd_data == 8'h62,
            "fall-through fifo stores stalled input");
        ft_rd_rdy = 1'b1;
        tick();
        ft_rd_rdy = 1'b0;
        #1;
        check(ft_empty, "fall-through fifo drains stored input");

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

        reset_all();
        ostk_alloc(2'd1, 8'hb1);
        ostk_alloc(2'd2, 8'hc1);
        ostk_alloc(2'd1, 8'hb2);
        ostk_alloc(2'd3, 8'hd1);
        ostk_lookup_key = 2'd2;
        #1;
        ostk_free_slot = ostk_lookup_slot;
        ostk_free_vld = 1'b1;
        tick();
        ostk_free_vld = 1'b0;
        ostk_alloc(2'd1, 8'hb3);
        ostk_lookup_key = 2'd1;
        #1;
        check(ostk_lookup_vld && ostk_lookup_ctx == 8'hb1,
            "ostk keeps oldest key entry across slot reuse");
        ostk_free_slot = ostk_lookup_slot;
        ostk_free_vld = 1'b1;
        tick();
        ostk_free_vld = 1'b0;
        #1;
        check(ostk_lookup_vld && ostk_lookup_ctx == 8'hb2,
            "ostk returns second key entry before reused slot");
        ostk_free_slot = ostk_lookup_slot;
        ostk_free_vld = 1'b1;
        tick();
        ostk_free_vld = 1'b0;
        #1;
        check(ostk_lookup_vld && ostk_lookup_ctx == 8'hb3,
            "ostk returns reused slot as newest key entry");

        slice_src_vld = 1'b1;
        slice_src_data = 8'h71;
        slice_dst_rdy = 1'b1;
        #1;
        check(slice_src_rdy && !slice_dst_vld,
            "reg slice captures input before presenting output");
        tick();
        slice_src_vld = 1'b0;
        #1;
        check(slice_dst_vld && slice_dst_data == 8'h71,
            "reg slice presents registered output");
        tick();
        slice_dst_rdy = 1'b0;
        #1;
        check(!slice_dst_vld, "reg slice drains direct transfer");

        slice_src_vld = 1'b1;
        slice_src_data = 8'h72;
        #1;
        check(slice_src_rdy && !slice_dst_vld,
            "reg slice accepts first stalled item");
        tick();
        slice_src_data = 8'h73;
        #1;
        check(slice_src_rdy && slice_dst_vld && slice_dst_data == 8'h72,
            "reg slice accepts second stalled item");
        tick();
        slice_src_data = 8'h74;
        #1;
        check(!slice_src_rdy && slice_dst_vld && slice_dst_data == 8'h72,
            "reg slice backpressures when full");
        slice_dst_rdy = 1'b1;
        tick();
        #1;
        check(slice_src_rdy && slice_dst_vld && slice_dst_data == 8'h73,
            "reg slice exposes skid item and releases backpressure");
        tick();
        slice_src_vld = 1'b0;
        #1;
        check(slice_dst_vld && slice_dst_data == 8'h74,
            "reg slice accepts next item after releasing backpressure");
        tick();
        slice_dst_rdy = 1'b0;
        #1;
        check(!slice_dst_vld, "reg slice empty after drain");

        slice_src_vld = 1'b1;
        slice_src_data = 8'h75;
        tick();
        slice_src_data = 8'h76;
        tick();
        slice_src_vld = 1'b0;
        #1;
        check(slice_dst_vld, "reg slice has data before clear");
        slice_clear = 1'b1;
        tick();
        slice_clear = 1'b0;
        #1;
        check(!slice_dst_vld && slice_src_rdy,
            "reg slice clear drops all buffered data");

        oldest_older_flat = '0;
        oldest_older_flat[1 * 4 + 0] = 1'b1;
        oldest_older_flat[1 * 4 + 2] = 1'b1;
        oldest_older_flat[1 * 4 + 3] = 1'b1;
        oldest_older_flat[2 * 4 + 0] = 1'b1;
        oldest_older_flat[2 * 4 + 3] = 1'b1;
        oldest_older_flat[3 * 4 + 0] = 1'b1;
        oldest_candidate_vld = 4'b1101;
        #1;
        check(oldest_select_vld && oldest_select_slot == 2'd2,
            "oldest select chooses oldest candidate");
        oldest_candidate_vld = '0;
        #1;
        check(!oldest_select_vld, "oldest select handles no candidate");

        $display("PASS: fifo_ost_tb");
        $finish;
    end
endmodule
