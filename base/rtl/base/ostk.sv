module ostk #(
    parameter int KEY_W = 1,
    parameter int DW = 1,
    parameter int DEPTH = 2,
    parameter int ISSUE_W = 32,
    localparam int PTR_W = $clog2(DEPTH)
)(
    input logic                 clk,
    input logic                 rst_n,

    input logic                 alloc_vld,
    output logic                alloc_rdy,
    input logic [KEY_W-1:0]     alloc_key,
    input logic [DW-1:0]        alloc_ctx,
    output logic [PTR_W-1:0]    alloc_slot,

    input logic [KEY_W-1:0]     lookup_key,
    output logic                lookup_vld,
    output logic [PTR_W-1:0]    lookup_slot,
    output logic [DW-1:0]       lookup_ctx,

    input logic                 free_vld,
    input logic [PTR_W-1:0]     free_slot,

    input logic                 slot_wr_vld,
    input logic [PTR_W-1:0]     slot_wr_idx,
    input logic [DW-1:0]        slot_wr_ctx,
    output logic [DEPTH-1:0]    slot_vld,
    output logic [DEPTH*KEY_W-1:0] slot_key_flat,
    output logic [DEPTH*DW-1:0] slot_ctx_flat,
    output logic [DEPTH*ISSUE_W-1:0] slot_seq_flat,

    output logic                empty,
    output logic                full
);
    logic [DEPTH-1:0] vld_mem;
    logic [KEY_W-1:0] key_mem[DEPTH];
    logic [DW-1:0] ctx_mem[DEPTH];
    logic [ISSUE_W-1:0] seq_mem[DEPTH];
    logic [ISSUE_W-1:0] next_issue_seq;
    logic [PTR_W-1:0] rptr;
    logic [PTR_W-1:0] wptr;
    logic [PTR_W:0] entry_count;

    logic alloc_found;
    logic lookup_found;
    logic [PTR_W-1:0] rptr_next;
    logic [PTR_W-1:0] wptr_next;
    logic [PTR_W:0] entry_count_next;

    wire alloc_hsk = alloc_vld & alloc_rdy;
    wire free_hsk = free_vld;
    wire free_head = free_hsk && (free_slot == rptr);

`ifndef SYNTHESIS
    initial begin
        assert (DEPTH >= 2)
            else $fatal(1, "ostk DEPTH must be >= 2");
        assert ((DEPTH & (DEPTH - 1)) == 0)
            else $fatal(1, "ostk DEPTH must be power-of-two");
    end
`endif

    always_comb begin
        alloc_found = 1'b0;
        alloc_slot = wptr;
        for (int i = 0; i < DEPTH; i++) begin
            logic [PTR_W-1:0] idx;
            idx = wptr + PTR_W'(i);
            if (!alloc_found && !vld_mem[idx]) begin
                alloc_found = 1'b1;
                alloc_slot = idx;
            end
        end
    end

    always_comb begin
        lookup_found = 1'b0;
        lookup_slot = '0;
        lookup_ctx = '0;
        for (int i = 0; i < DEPTH; i++) begin
            logic [PTR_W-1:0] idx;
            idx = rptr + PTR_W'(i);
            if (!lookup_found && vld_mem[idx] && key_mem[idx] == lookup_key) begin
                lookup_found = 1'b1;
                lookup_slot = idx;
                lookup_ctx = ctx_mem[idx];
            end
        end
    end

    always_comb begin
        wptr_next = wptr;
        if (alloc_hsk)
            wptr_next = alloc_slot + 1'b1;

        entry_count_next = entry_count;
        unique case ({alloc_hsk, free_hsk})
            2'b10: entry_count_next = entry_count + 1'b1;
            2'b01: entry_count_next = entry_count - 1'b1;
            default: entry_count_next = entry_count;
        endcase

        rptr_next = rptr;
        if (entry_count_next == '0) begin
            rptr_next = wptr_next;
        end else if (entry_count == '0 && alloc_hsk) begin
            rptr_next = alloc_slot;
        end else if (free_head) begin
            rptr_next = rptr + 1'b1;
        end
    end

    assign empty = entry_count == '0;
    assign full = entry_count == (PTR_W + 1)'(DEPTH);
    assign alloc_rdy = alloc_found;
    assign lookup_vld = lookup_found;
    assign slot_vld = vld_mem;

    for (genvar i = 0; i < DEPTH; i++) begin
        assign slot_key_flat[i * KEY_W +: KEY_W] = key_mem[i];
        assign slot_ctx_flat[i * DW +: DW] = ctx_mem[i];
        assign slot_seq_flat[i * ISSUE_W +: ISSUE_W] = seq_mem[i];
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            vld_mem <= '0;
            next_issue_seq <= '0;
            rptr <= '0;
            wptr <= '0;
            entry_count <= '0;
        end else begin
            if (alloc_hsk) begin
                vld_mem[alloc_slot] <= 1'b1;
                key_mem[alloc_slot] <= alloc_key;
                ctx_mem[alloc_slot] <= alloc_ctx;
                seq_mem[alloc_slot] <= next_issue_seq;
                next_issue_seq <= next_issue_seq + 1'b1;
            end
            if (slot_wr_vld && vld_mem[slot_wr_idx])
                ctx_mem[slot_wr_idx] <= slot_wr_ctx;
            if (free_hsk)
                vld_mem[free_slot] <= 1'b0;
            rptr <= rptr_next;
            wptr <= wptr_next;
            entry_count <= entry_count_next;
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && free_vld) begin
            assert (vld_mem[free_slot])
                else $fatal(1, "ostk free invalid slot");
        end
    end
`endif
endmodule
