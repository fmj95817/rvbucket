`timescale 1ns/1ps

`include "itf/bti_req_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_r_if.svh"

module l1_cmo_tb;
    localparam logic [31:0] PAGE_BASE = 32'h4001_0000;
    localparam logic [31:0] DEST_BASE = 32'h4002_0000;
    localparam int PAGE_BYTES = 4096;
    localparam int WORDS = PAGE_BYTES / 4;
    localparam int TOTAL_WORDS = 2 * WORDS;
    localparam int LINES = PAGE_BYTES / 64;

    logic clk;
    logic rst_n;

    bti_req_if_t host_req();
    bti_rsp_if_t host_rsp();
    l1_flush_if_t flush();
    l1_flush_ack_if_t flush_ack();
    axi4_aw_if_t mem_aw();
    axi4_w_if_t mem_w();
    axi4_b_if_t mem_b();
    axi4_ar_if_t mem_ar();
    axi4_r_if_t mem_r();
    perf_l1_if_t perf();

    logic [31:0] backing_mem[0:TOTAL_WORDS-1];
    logic rd_active;
    logic [7:0] rd_id;
    logic [31:0] rd_addr;
    logic [7:0] rd_len;
    logic [7:0] rd_beat;
    logic rd_cmo;

    l1 u_dut(
        .clk              (clk),
        .rst_n            (rst_n),
        .host_bti_req_slv (host_req),
        .host_bti_rsp_mst (host_rsp),
        .flush_slv        (flush),
        .flush_ack_mst    (flush_ack),
        .mem_axi4_aw_mst  (mem_aw),
        .mem_axi4_w_mst   (mem_w),
        .mem_axi4_b_slv   (mem_b),
        .mem_axi4_ar_mst  (mem_ar),
        .mem_axi4_r_slv   (mem_r),
        .perf_mst         (perf)
    );

    function automatic logic [31:0] new_word(input int unsigned idx);
        if (idx == 32'h194 / 4)
            new_word = 32'h0009_348c;
        else
            new_word = 32'h5a00_0000 ^ (32'(idx) * 32'h0001_0201);
    endfunction

    function automatic logic addr_in_page(input logic [31:0] addr);
        addr_in_page = (addr >= PAGE_BASE &&
            addr < PAGE_BASE + PAGE_BYTES) ||
            (addr >= DEST_BASE && addr < DEST_BASE + PAGE_BYTES);
    endfunction

    function automatic logic [31:0] read_backing(
        input logic [31:0] addr
    );
        int unsigned idx;
        begin
            if (!addr_in_page(addr))
                return 32'b0;
            idx = addr >= DEST_BASE ?
                WORDS + ((addr - DEST_BASE) >> 2) :
                ((addr - PAGE_BASE) >> 2);
            read_backing = backing_mem[idx];
        end
    endfunction

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        #2_000_000;
        $display("FAIL: l1_cmo_tb watchdog timeout");
        $finish;
    end

    assign host_rsp.rdy = 1'b1;
    assign flush.vld = 1'b0;

    assign mem_ar.rdy = !rd_active;
    assign mem_r.vld = rd_active;
    assign mem_r.pkt.id = rd_id;
    assign mem_r.pkt.data = rd_cmo ? 32'b0 :
        read_backing(rd_addr + ({24'b0, rd_beat} << 2));
    assign mem_r.pkt.resp = AXI4_R_RESP_OKAY;
    assign mem_r.pkt.last = rd_beat == rd_len;

    assign mem_aw.rdy = 1'b1;
    assign mem_w.rdy = 1'b1;
    assign mem_b.vld = 1'b0;
    assign mem_b.pkt = '0;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_active <= 1'b0;
            rd_id <= '0;
            rd_addr <= '0;
            rd_len <= '0;
            rd_beat <= '0;
            rd_cmo <= 1'b0;
        end else begin
            if (mem_ar.vld && mem_ar.rdy) begin
                if (!addr_in_page(mem_ar.pkt.addr)) begin
                    $display("FAIL: AXI read outside test page addr=%08x",
                        mem_ar.pkt.addr);
                    $finish;
                end
                rd_active <= 1'b1;
                rd_id <= mem_ar.pkt.id;
                rd_addr <= mem_ar.pkt.addr;
                rd_len <= mem_ar.pkt.len;
                rd_beat <= '0;
                rd_cmo <= `AXI4_USER_IS_CMO(mem_ar.pkt.user);
            end else if (mem_r.vld && mem_r.rdy) begin
                if (mem_r.pkt.last)
                    rd_active <= 1'b0;
                else
                    rd_beat <= rd_beat + 1'b1;
            end
        end
    end

    always_ff @(posedge clk) begin
        if (rst_n && (mem_aw.vld || mem_w.vld)) begin
            $display("FAIL: unexpected L1 writeback aw_vld=%0b w_vld=%0b",
                mem_aw.vld, mem_w.vld);
            $finish;
        end
    end

    task automatic tick(input int unsigned count = 1);
        repeat (count) @(posedge clk);
    endtask

    task automatic request(
        input bti_req_cmd_t cmd,
        input logic [31:0] addr,
        input logic [31:0] req_data,
        input logic [31:0] expected_data
    );
        logic [15:0] trans_id;
        begin
            trans_id = addr[15:0] ^ {13'b0, cmd};
            @(negedge clk);
            host_req.pkt.trans_id = trans_id;
            host_req.pkt.cmd = cmd;
            host_req.pkt.addr = addr;
            host_req.pkt.size = BTI_REQ_SIZE_B4;
            host_req.pkt.data = req_data;
            host_req.pkt.strobe = cmd == BTI_REQ_CMD_WRITE ? 4'hf : 4'h0;
            host_req.vld = 1'b1;
            do begin
                tick();
            end while (!host_req.rdy);
            @(negedge clk);
            host_req.vld = 1'b0;

            while (!host_rsp.vld)
                tick();
            if (!host_rsp.pkt.ok || host_rsp.pkt.trans_id !== trans_id ||
                host_rsp.pkt.data !== expected_data) begin
                $display("FAIL: cmd=%0d addr=%08x exp=%08x got=%08x ok=%0b exp_id=%04x got_id=%04x",
                    cmd, addr, expected_data, host_rsp.pkt.data,
                    host_rsp.pkt.ok, trans_id, host_rsp.pkt.trans_id);
                $finish;
            end
            tick();
        end
    endtask

    initial begin
        rst_n = 1'b0;
        host_req.vld = 1'b0;
        host_req.pkt = '0;
        for (int unsigned i = 0; i < TOTAL_WORDS; i++)
            backing_mem[i] = 32'h0000_0023;

        tick(5);
        rst_n = 1'b1;
        tick(3);

        // Populate every test-page line with stale pre-DMA data.
        for (int unsigned line = 0; line < LINES; line++) begin
            request(BTI_REQ_CMD_READ, PAGE_BASE + line * 64,
                32'b0, 32'h0000_0023);
            request(BTI_REQ_CMD_READ, DEST_BASE + line * 64,
                32'b0, 32'h0000_0023);
        end

        // Model an external DMA overwrite that does not update L1D.
        @(negedge clk);
        for (int unsigned i = 0; i < WORDS; i++)
            backing_mem[i] = new_word(i);

        for (int unsigned line = 0; line < LINES; line++)
            request(BTI_REQ_CMD_CBO_INVAL, PAGE_BASE + line * 64,
                32'b0, 32'b0);

        // Model Linux copying the DMA page into a private writable mapping.
        for (int unsigned i = 0; i < WORDS; i++) begin
            request(BTI_REQ_CMD_READ, PAGE_BASE + i * 4,
                32'b0, new_word(i));
            request(BTI_REQ_CMD_WRITE, DEST_BASE + i * 4,
                new_word(i), 32'b0);
        end

        for (int unsigned i = 0; i < WORDS; i++)
            request(BTI_REQ_CMD_READ, DEST_BASE + i * 4,
                32'b0, new_word(i));

        $display("PASS: l1_cmo_tb");
        $finish;
    end
endmodule
