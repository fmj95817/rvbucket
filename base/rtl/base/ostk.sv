module ostk #(
    parameter int KEY_W = 1,
    parameter int DW = 1,
    parameter int DEPTH = 2,
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
    output logic [DEPTH*DEPTH-1:0] slot_older_flat,

    output logic                empty,
    output logic                full
);
    logic [DEPTH-1:0] vld_mem;
    logic [KEY_W-1:0] key_mem[DEPTH];
    logic [DW-1:0] ctx_mem[DEPTH];
    logic [PTR_W-1:0] free_mem[DEPTH];
    // older_mem[i][j] is set while valid slot i was allocated before slot j.
    logic [DEPTH-1:0] older_mem[DEPTH];
    logic [PTR_W:0] free_rptr;
    logic [PTR_W:0] free_wptr;
    logic [PTR_W:0] entry_count;

    logic [DEPTH-1:0] lookup_candidate;
    logic [DEPTH-1:0] lookup_head;
    logic [PTR_W:0] entry_count_next;
    logic free_empty;
    logic free_full;

    wire alloc_hsk = alloc_vld & alloc_rdy;
    wire free_hsk = free_vld;

`ifndef SYNTHESIS
    initial begin
        assert (DEPTH >= 2)
            else $fatal(1, "ostk DEPTH must be >= 2");
        assert ((DEPTH & (DEPTH - 1)) == 0)
            else $fatal(1, "ostk DEPTH must be power-of-two");
    end
`endif

    assign free_empty = free_rptr == free_wptr;
    assign free_full = free_rptr[PTR_W] != free_wptr[PTR_W] &&
        free_rptr[PTR_W-1:0] == free_wptr[PTR_W-1:0];
    assign alloc_slot = free_mem[free_rptr[PTR_W-1:0]];

    for (genvar i = 0; i < DEPTH; i++) begin : gen_lookup_candidate
        logic [DEPTH-1:0] older_candidate;

        assign lookup_candidate[i] = vld_mem[i] &&
            key_mem[i] == lookup_key;
        for (genvar j = 0; j < DEPTH; j++) begin : gen_older_candidate
            assign older_candidate[j] = lookup_candidate[j] &&
                older_mem[j][i];
        end

        assign lookup_head[i] = lookup_candidate[i] && !(|older_candidate);
    end

    always_comb begin
        lookup_vld = 1'b0;
        lookup_slot = '0;
        lookup_ctx = '0;
        for (int i = 0; i < DEPTH; i++) begin
            if (!lookup_vld && lookup_head[i]) begin
                lookup_vld = 1'b1;
                lookup_slot = PTR_W'(i);
                lookup_ctx = ctx_mem[i];
            end
        end
    end

    always_comb begin
        entry_count_next = entry_count;
        unique case ({alloc_hsk, free_hsk})
            2'b10: entry_count_next = entry_count + 1'b1;
            2'b01: entry_count_next = entry_count - 1'b1;
            default: entry_count_next = entry_count;
        endcase

    end

    assign empty = entry_count == '0;
    assign full = entry_count == (PTR_W + 1)'(DEPTH);
    assign alloc_rdy = !free_empty;
    assign slot_vld = vld_mem;

    for (genvar i = 0; i < DEPTH; i++) begin
        assign slot_key_flat[i * KEY_W +: KEY_W] = key_mem[i];
        assign slot_ctx_flat[i * DW +: DW] = ctx_mem[i];
        assign slot_older_flat[i * DEPTH +: DEPTH] = older_mem[i];
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            vld_mem <= '0;
            free_rptr <= '0;
            free_wptr <= (PTR_W + 1)'(DEPTH);
            entry_count <= '0;
            for (int i = 0; i < DEPTH; i++) begin
                free_mem[i] <= PTR_W'(i);
                older_mem[i] <= '0;
            end
        end else begin
            if (alloc_hsk)
                free_rptr <= free_rptr + 1'b1;
            if (free_hsk) begin
                free_mem[free_wptr[PTR_W-1:0]] <= free_slot;
                free_wptr <= free_wptr + 1'b1;
            end
            if (free_hsk) begin
                for (int i = 0; i < DEPTH; i++) begin
                    older_mem[free_slot][i] <= 1'b0;
                    older_mem[i][free_slot] <= 1'b0;
                end
            end
            if (alloc_hsk) begin
                vld_mem[alloc_slot] <= 1'b1;
                key_mem[alloc_slot] <= alloc_key;
                ctx_mem[alloc_slot] <= alloc_ctx;
                for (int i = 0; i < DEPTH; i++) begin
                    older_mem[alloc_slot][i] <= 1'b0;
                    older_mem[i][alloc_slot] <= PTR_W'(i) != alloc_slot &&
                        vld_mem[i] && !(free_hsk && free_slot == PTR_W'(i));
                end
            end
            if (slot_wr_vld && vld_mem[slot_wr_idx])
                ctx_mem[slot_wr_idx] <= slot_wr_ctx;
            if (free_hsk)
                vld_mem[free_slot] <= 1'b0;
            entry_count <= entry_count_next;
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && free_vld) begin
            assert (vld_mem[free_slot])
                else $fatal(1, "ostk free invalid slot");
        end
        if (rst_n && free_hsk) begin
            assert (!free_full || alloc_hsk)
                else $fatal(1, "ostk free-list overflow");
        end
    end
`endif

    wire unused = free_full;
endmodule
