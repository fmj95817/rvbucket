`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi_demux_rsp_slice_tb;
    localparam int GST_NUM = 2;
    localparam int REQ_NUM = 256;
    localparam int WR_REQ_NUM = 64;
    localparam int ID_NUM = 4;
    localparam int QUEUE_DEPTH = 256;
    localparam int QUEUE_PTR_W = $clog2(QUEUE_DEPTH);

    logic clk;
    logic rst_n;

    axi4_aw_if_t host_aw();
    axi4_w_if_t host_w();
    axi4_b_if_t host_b();
    axi4_ar_if_t host_ar();
    axi4_r_if_t host_r();
    axi4_aw_if_t gst_aw[GST_NUM]();
    axi4_w_if_t gst_w[GST_NUM]();
    axi4_b_if_t gst_b[GST_NUM]();
    axi4_ar_if_t gst_ar[GST_NUM]();
    axi4_r_if_t gst_r[GST_NUM]();
    axi4_b_if_t raw_b[GST_NUM]();
    axi4_r_if_t raw_r[GST_NUM]();
    perf_axi_demux_if_t perf();

    logic [47:0] rsp_queue[GST_NUM][QUEUE_DEPTH];
    logic [QUEUE_PTR_W-1:0] queue_wptr[GST_NUM];
    logic [QUEUE_PTR_W-1:0] queue_rptr[GST_NUM];
    logic [QUEUE_PTR_W:0] queue_count[GST_NUM];
    logic [15:0] guest_lfsr[GST_NUM];
    logic [7:0] guest_rsp_beat[GST_NUM];
    logic [7:0] wr_aw_queue[GST_NUM][QUEUE_DEPTH];
    logic [QUEUE_PTR_W-1:0] wr_aw_wptr[GST_NUM];
    logic [QUEUE_PTR_W-1:0] wr_aw_rptr[GST_NUM];
    logic [QUEUE_PTR_W:0] wr_aw_count[GST_NUM];
    logic [7:0] wr_b_queue[GST_NUM][QUEUE_DEPTH];
    logic [QUEUE_PTR_W-1:0] wr_b_wptr[GST_NUM];
    logic [QUEUE_PTR_W-1:0] wr_b_rptr[GST_NUM];
    logic [QUEUE_PTR_W:0] wr_b_count[GST_NUM];
    logic [15:0] host_lfsr;
    logic host_r_stalled;
    logic [$bits(host_r.pkt)-1:0] host_r_stalled_pkt;
    logic host_b_stalled;
    logic [$bits(host_b.pkt)-1:0] host_b_stalled_pkt;
    int unsigned expected_seq[ID_NUM];
    int unsigned expected_beat[ID_NUM];
    int unsigned expected_rsp_count;
    int unsigned rsp_count;
    logic [WR_REQ_NUM-1:0] b_seen;
    int unsigned b_count;

    axi_demux #(
        .GST_NUM        (GST_NUM),
        .GST_BASE       ('{0: 32'h00001000, 1: 32'h00002000,
            default: 32'h00000000}),
        .GST_SIZE       ('{0: 32'h00001000, 1: 32'h00001000,
            default: 32'h00000000}),
        .STG_FIFO_DEPTH (4),
        .OST_DEPTH      (8)
    ) u_dut(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_axi4_aw_slv (host_aw),
        .host_axi4_w_slv  (host_w),
        .host_axi4_b_mst  (host_b),
        .host_axi4_ar_slv (host_ar),
        .host_axi4_r_mst  (host_r),
        .gst_axi4_aw_msts (gst_aw),
        .gst_axi4_w_msts  (gst_w),
        .gst_axi4_b_slvs  (gst_b),
        .gst_axi4_ar_msts (gst_ar),
        .gst_axi4_r_slvs  (gst_r),
        .perf_mst         (perf)
    );

    for (genvar i = 0; i < GST_NUM; i++) begin : gen_guest
        logic queue_push;
        logic rsp_complete;
        logic wr_aw_push;
        logic wr_w_pop;
        logic wr_b_pop;

        axi_rsp_reg_slice u_rsp_slice(
            .clk             (clk),
            .rst_n           (rst_n),
            .host_axi4_b_mst (gst_b[i]),
            .host_axi4_r_mst (gst_r[i]),
            .gst_axi4_b_slv  (raw_b[i]),
            .gst_axi4_r_slv  (raw_r[i])
        );

        assign gst_aw[i].rdy = 1'b1;
        assign gst_w[i].rdy = 1'b1;
        assign gst_ar[i].rdy = queue_count[i] != QUEUE_DEPTH;

        assign queue_push = gst_ar[i].vld && gst_ar[i].rdy;
        assign rsp_complete = raw_r[i].vld && raw_r[i].rdy &&
            raw_r[i].pkt.last;
        assign wr_aw_push = gst_aw[i].vld && gst_aw[i].rdy;
        assign wr_w_pop = gst_w[i].vld && gst_w[i].rdy &&
            gst_w[i].pkt.last;
        assign wr_b_pop = raw_b[i].vld && raw_b[i].rdy;

        always_ff @(posedge clk or negedge rst_n) begin
            if (!rst_n) begin
                queue_wptr[i] <= '0;
                queue_rptr[i] <= '0;
                queue_count[i] <= '0;
                guest_lfsr[i] <= 16'h1ace ^ (16'(i) << 4);
                guest_rsp_beat[i] <= '0;
                raw_r[i].vld <= 1'b0;
                raw_r[i].pkt <= '0;
                wr_aw_wptr[i] <= '0;
                wr_aw_rptr[i] <= '0;
                wr_aw_count[i] <= '0;
                wr_b_wptr[i] <= '0;
                wr_b_rptr[i] <= '0;
                wr_b_count[i] <= '0;
                raw_b[i].vld <= 1'b0;
                raw_b[i].pkt <= '0;
            end else begin
                guest_lfsr[i] <= {guest_lfsr[i][14:0],
                    guest_lfsr[i][15] ^ guest_lfsr[i][13] ^
                    guest_lfsr[i][12] ^ guest_lfsr[i][10]};

                if (queue_push) begin
                    rsp_queue[i][queue_wptr[i]] <= {
                        gst_ar[i].pkt.id, gst_ar[i].pkt.len,
                        gst_ar[i].pkt.user};
                    queue_wptr[i] <= queue_wptr[i] + 1'b1;
                end

                if (raw_r[i].vld && raw_r[i].rdy) begin
                    raw_r[i].vld <= 1'b0;
                    if (raw_r[i].pkt.last) begin
                        queue_rptr[i] <= queue_rptr[i] + 1'b1;
                        guest_rsp_beat[i] <= '0;
                    end else begin
                        guest_rsp_beat[i] <= guest_rsp_beat[i] + 1'b1;
                    end
                end

                if (!raw_r[i].vld && queue_count[i] != 0 &&
                    guest_lfsr[i][0]) begin
                    raw_r[i].vld <= 1'b1;
                    raw_r[i].pkt.id <=
                        rsp_queue[i][queue_rptr[i]][47:40];
                    raw_r[i].pkt.data <= 32'ha5000000 |
                        (rsp_queue[i][queue_rptr[i]][15:0] << 8) |
                        guest_rsp_beat[i];
                    raw_r[i].pkt.resp <= AXI4_R_RESP_OKAY;
                    raw_r[i].pkt.last <= guest_rsp_beat[i] ==
                        rsp_queue[i][queue_rptr[i]][39:32];
                end

                unique case ({queue_push, rsp_complete})
                2'b10: queue_count[i] <= queue_count[i] + 1'b1;
                2'b01: queue_count[i] <= queue_count[i] - 1'b1;
                default: queue_count[i] <= queue_count[i];
                endcase

                if (wr_aw_push) begin
                    wr_aw_queue[i][wr_aw_wptr[i]] <= gst_aw[i].pkt.id;
                    wr_aw_wptr[i] <= wr_aw_wptr[i] + 1'b1;
                end

                if (wr_w_pop) begin
                    wr_b_queue[i][wr_b_wptr[i]] <=
                        wr_aw_queue[i][wr_aw_rptr[i]];
                    wr_aw_rptr[i] <= wr_aw_rptr[i] + 1'b1;
                    wr_b_wptr[i] <= wr_b_wptr[i] + 1'b1;
                end

                unique case ({wr_aw_push, wr_w_pop})
                2'b10: wr_aw_count[i] <= wr_aw_count[i] + 1'b1;
                2'b01: wr_aw_count[i] <= wr_aw_count[i] - 1'b1;
                default: wr_aw_count[i] <= wr_aw_count[i];
                endcase

                if (wr_b_pop) begin
                    raw_b[i].vld <= 1'b0;
                    wr_b_rptr[i] <= wr_b_rptr[i] + 1'b1;
                end

                if (!raw_b[i].vld && wr_b_count[i] != 0 &&
                    guest_lfsr[i][2]) begin
                    raw_b[i].vld <= 1'b1;
                    raw_b[i].pkt.id <= wr_b_queue[i][wr_b_rptr[i]];
                    raw_b[i].pkt.resp <= AXI4_B_RESP_OKAY;
                end

                unique case ({wr_w_pop, wr_b_pop})
                2'b10: wr_b_count[i] <= wr_b_count[i] + 1'b1;
                2'b01: wr_b_count[i] <= wr_b_count[i] - 1'b1;
                default: wr_b_count[i] <= wr_b_count[i];
                endcase
            end
        end

`ifndef SYNTHESIS
        always_ff @(posedge clk) begin
            if (rst_n && wr_w_pop)
                assert (wr_aw_count[i] != 0)
                    else $fatal(1, "guest W arrived without AW");
        end
`endif
    end

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    task automatic tick(input int unsigned n = 1);
        repeat (n) begin
            @(posedge clk);
            #1;
        end
    endtask

    function automatic logic [7:0] burst_len(input int unsigned seq);
        unique case (seq % 4)
        0: burst_len = 8'd0;
        1: burst_len = 8'd3;
        2: burst_len = 8'd7;
        default: burst_len = 8'd15;
        endcase
    endfunction

    task automatic send_ar(input int unsigned seq);
        logic [31:0] base;
        begin
            base = seq[0] ? 32'h00002000 : 32'h00001000;
            host_ar.pkt.id = 8'(seq % ID_NUM);
            host_ar.pkt.addr = base + (seq << 2);
            host_ar.pkt.len = burst_len(seq);
            host_ar.pkt.size = AXI4_AR_SIZE_B4;
            host_ar.pkt.burst = AXI4_AR_BURST_INCR;
            host_ar.pkt.lock = 1'b0;
            host_ar.pkt.cache = 4'h0;
            host_ar.pkt.prot = 3'h0;
            host_ar.pkt.qos = 4'h0;
            host_ar.pkt.user = 32'(seq);
            host_ar.vld = 1'b1;
            do @(posedge clk); while (!host_ar.rdy);
            #1;
            host_ar.vld = 1'b0;
            expected_rsp_count = expected_rsp_count + burst_len(seq) + 1;
        end
    endtask

    task automatic send_write(input int unsigned seq);
        logic [31:0] base;
        begin
            base = seq[0] ? 32'h00002000 : 32'h00001000;
            host_aw.pkt.id = 8'(seq);
            host_aw.pkt.addr = base + (seq << 2);
            host_aw.pkt.len = 8'd0;
            host_aw.pkt.size = AXI4_AW_SIZE_B4;
            host_aw.pkt.burst = AXI4_AW_BURST_INCR;
            host_aw.pkt.lock = 1'b0;
            host_aw.pkt.cache = 4'h0;
            host_aw.pkt.prot = 3'h0;
            host_aw.pkt.qos = 4'h0;
            host_aw.pkt.user = 32'(seq);
            host_aw.vld = 1'b1;
            do @(posedge clk); while (!host_aw.rdy);
            #1;
            host_aw.vld = 1'b0;

            host_w.pkt.data = 32'h5a000000 | seq;
            host_w.pkt.strb = 4'hf;
            host_w.pkt.last = 1'b1;
            host_w.vld = 1'b1;
            do @(posedge clk); while (!host_w.rdy);
            #1;
            host_w.vld = 1'b0;
        end
    endtask

    initial begin
        #2000000;
        $display("FAIL: axi_demux_rsp_slice_tb watchdog rsp_count=%0d",
            rsp_count);
        $fatal(1);
    end

    initial begin
        rst_n = 1'b0;
        expected_rsp_count = 0;
        host_aw.vld = 1'b0;
        host_w.vld = 1'b0;
        host_ar.vld = 1'b0;
        host_ar.pkt = '0;

        tick(4);
        rst_n = 1'b1;
        tick(2);

        for (int unsigned i = 0; i < REQ_NUM; i++)
            send_ar(i);

        wait (rsp_count == expected_rsp_count);
        for (int unsigned i = 0; i < WR_REQ_NUM; i++)
            send_write(i);

        wait (b_count == WR_REQ_NUM);
        tick(4);
        $display("PASS: axi_demux_rsp_slice_tb");
        $finish;
    end

    assign host_r.rdy = host_lfsr[0] || host_lfsr[3];
    assign host_b.rdy = host_lfsr[1] || host_lfsr[4];

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            host_lfsr <= 16'hc001;
            rsp_count <= 0;
            host_r_stalled <= 1'b0;
            host_r_stalled_pkt <= '0;
            host_b_stalled <= 1'b0;
            host_b_stalled_pkt <= '0;
            b_seen <= '0;
            b_count <= 0;
            for (int unsigned i = 0; i < ID_NUM; i++) begin
                expected_seq[i] <= i;
                expected_beat[i] <= 0;
            end
        end else begin
            host_lfsr <= {host_lfsr[14:0], host_lfsr[15] ^
                host_lfsr[13] ^ host_lfsr[12] ^ host_lfsr[10]};

            if (host_r.vld && host_r.rdy) begin
                int unsigned id;
                int unsigned seq;
                int unsigned beat;
                id = int'(host_r.pkt.id);
                seq = int'((host_r.pkt.data >> 8) & 32'h0000ffff);
                beat = int'(host_r.pkt.data & 32'h000000ff);
                if (id >= ID_NUM || seq != expected_seq[id] ||
                    beat != expected_beat[id] ||
                    host_r.pkt.data[31:24] != 8'ha5 ||
                    host_r.pkt.resp != AXI4_R_RESP_OKAY ||
                    host_r.pkt.last != (beat == burst_len(seq))) begin
                    $display("FAIL: response id=%0d seq=%0d exp=%0d beat=%0d exp_beat=%0d data=%08x last=%b",
                        id, seq, id < ID_NUM ? expected_seq[id] : 32'hffffffff,
                        beat, id < ID_NUM ? expected_beat[id] : 32'hffffffff,
                        host_r.pkt.data, host_r.pkt.last);
                    $fatal(1);
                end
                if (host_r.pkt.last) begin
                    expected_seq[id] <= expected_seq[id] + ID_NUM;
                    expected_beat[id] <= 0;
                end else begin
                    expected_beat[id] <= expected_beat[id] + 1;
                end
                rsp_count <= rsp_count + 1;
            end

            if (host_b.vld && host_b.rdy) begin
                int unsigned id;
                id = int'(host_b.pkt.id);
                if (id >= WR_REQ_NUM || b_seen[id] ||
                    host_b.pkt.resp != AXI4_B_RESP_OKAY) begin
                    $display("FAIL: B id=%0d seen=%b resp=%0d",
                        id, id < WR_REQ_NUM ? b_seen[id] : 1'bx,
                        host_b.pkt.resp);
                    $fatal(1);
                end
                b_seen[id] <= 1'b1;
                b_count <= b_count + 1;
            end

            if (host_r_stalled && (!host_r.vld ||
                host_r.pkt !== host_r_stalled_pkt)) begin
                $display("FAIL: host R changed while stalled old=%x new=%x vld=%b",
                    host_r_stalled_pkt, host_r.pkt, host_r.vld);
                $display("  rr=%0d sel_vld=%b sel_idx=%0d sel_slot=%0d rdy=%b",
                    u_dut.rd_rsp_rr_idx, u_dut.rd_rsp_sel_vld,
                    u_dut.rd_rsp_sel_idx, u_dut.rd_rsp_sel_slot,
                    host_r.rdy);
                $display("  guest0 vld=%b rdy=%b pkt=%x match_vld=%b match_slot=%0d",
                    gst_r[0].vld, gst_r[0].rdy, gst_r[0].pkt,
                    u_dut.rd_rsp_match_vld[0],
                    u_dut.rd_rsp_match_slot[0]);
                $display("  guest1 vld=%b rdy=%b pkt=%x match_vld=%b match_slot=%0d",
                    gst_r[1].vld, gst_r[1].rdy, gst_r[1].pkt,
                    u_dut.rd_rsp_match_vld[1],
                    u_dut.rd_rsp_match_slot[1]);
                $fatal(1);
            end
            host_r_stalled <= host_r.vld && !host_r.rdy;
            if (host_r.vld && !host_r.rdy)
                host_r_stalled_pkt <= host_r.pkt;

            if (host_b_stalled && (!host_b.vld ||
                host_b.pkt !== host_b_stalled_pkt)) begin
                $display("FAIL: host B changed while stalled old=%x new=%x vld=%b",
                    host_b_stalled_pkt, host_b.pkt, host_b.vld);
                $fatal(1);
            end
            host_b_stalled <= host_b.vld && !host_b.rdy;
            if (host_b.vld && !host_b.rdy)
                host_b_stalled_pkt <= host_b.pkt;
        end
    end
endmodule
