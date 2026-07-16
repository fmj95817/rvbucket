module ostq #(
    parameter int DW = 1,
    parameter int DEPTH = 2,
    localparam int PTR_W = $clog2(DEPTH)
)(
    input logic             clk,
    input logic             rst_n,

    input logic             alloc_vld,
    output logic            alloc_rdy,
    input logic [DW-1:0]    alloc_ctx,
    output logic [PTR_W-1:0] alloc_slot,

    output logic            head_vld,
    output logic [DW-1:0]   head_ctx,
    output logic [PTR_W-1:0] head_slot,
    input logic             free_head,

    input logic             slot_wr_vld,
    input logic [PTR_W-1:0] slot_wr_idx,
    input logic [DW-1:0]    slot_wr_ctx,
    output logic [DEPTH-1:0] slot_vld,
    output logic [DEPTH*DW-1:0] slot_ctx_flat,

    output logic            empty,
    output logic            full
);
    logic [DW-1:0] ctx_mem[DEPTH];
    logic [DEPTH-1:0] vld_mem;
    logic [PTR_W:0] rptr;
    logic [PTR_W:0] wptr;
    logic [PTR_W-1:0] alloc_slot_idx;
    logic [PTR_W-1:0] head_slot_idx;

    wire ptr_msb_diff = rptr[PTR_W] != wptr[PTR_W];
    wire ptr_lsb_same = rptr[PTR_W-1:0] == wptr[PTR_W-1:0];
    wire alloc_hsk = alloc_vld & alloc_rdy;
    wire free_hsk = free_head & head_vld;

`ifndef SYNTHESIS
    initial begin
        assert (DEPTH >= 2)
            else $fatal(1, "ostq DEPTH must be >= 2");
        assert ((DEPTH & (DEPTH - 1)) == 0)
            else $fatal(1, "ostq DEPTH must be power-of-two");
    end
`endif

    assign empty = rptr == wptr;
    assign full = ptr_msb_diff & ptr_lsb_same;
    assign alloc_rdy = !full;
    assign alloc_slot_idx = wptr[PTR_W-1:0];
    assign head_slot_idx = rptr[PTR_W-1:0];
    assign alloc_slot = alloc_slot_idx;
    assign head_slot = head_slot_idx;
    assign head_vld = !empty & vld_mem[head_slot_idx];
    assign head_ctx = head_vld ? ctx_mem[head_slot_idx] : {DW{1'b0}};
    assign slot_vld = vld_mem;

    for (genvar i = 0; i < DEPTH; i++) begin
        assign slot_ctx_flat[i * DW +: DW] = ctx_mem[i];
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            vld_mem <= '0;
        end else begin
            if (alloc_hsk)
                vld_mem[alloc_slot_idx] <= 1'b1;
            if (free_hsk)
                vld_mem[head_slot_idx] <= 1'b0;
        end
    end

    always_ff @(posedge clk) begin
        if (alloc_hsk)
            ctx_mem[alloc_slot_idx] <= alloc_ctx;
        if (slot_wr_vld && vld_mem[slot_wr_idx] &&
            !(alloc_hsk && alloc_slot_idx == slot_wr_idx))
            ctx_mem[slot_wr_idx] <= slot_wr_ctx;
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rptr <= '0;
            wptr <= '0;
        end else begin
            if (alloc_hsk)
                wptr <= wptr + 1'b1;
            if (free_hsk)
                rptr <= rptr + 1'b1;
        end
    end
endmodule
