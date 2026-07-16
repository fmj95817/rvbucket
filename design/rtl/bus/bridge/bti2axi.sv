`include "itf/bti_req_if.svh"
`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module bti2axi #(
    parameter int STG_FIFO_DEPTH = 4,
    parameter int OST_DEPTH = 8
)(
    input logic       clk,
    input logic       rst_n,
    bti_req_if_t.slv  bti_req_slv,
    bti_rsp_if_t.mst  bti_rsp_mst,
    axi4_aw_if_t.mst  axi4_aw_mst,
    axi4_w_if_t.mst   axi4_w_mst,
    axi4_b_if_t.slv   axi4_b_slv,
    axi4_ar_if_t.mst  axi4_ar_mst,
    axi4_r_if_t.slv   axi4_r_slv
);
    typedef struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        bti_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } bti_req_pkt_t;

    typedef struct packed {
        logic [15:0] bti_trans_id;
    } ost_ctx_t;

    localparam int BTI_REQ_DW = $bits(bti_req_pkt_t);
    localparam int OST_CTX_DW = $bits(ost_ctx_t);
    localparam int RD_OST_SLOT_W = $clog2(OST_DEPTH);
    localparam int WR_OST_SLOT_W = $clog2(OST_DEPTH);

    logic req_fifo_rd_vld;
    logic req_fifo_rd_rdy;
    logic [BTI_REQ_DW-1:0] req_fifo_rd_data;
    bti_req_pkt_t req_pkt;

    logic req_is_read;
    logic rd_issue_hsk;
    logic wr_issue_hsk;
    logic wr_aw_sent;
    logic wr_w_sent;

    logic rd_ost_alloc_rdy;
    logic [RD_OST_SLOT_W-1:0] rd_ost_alloc_slot;
    logic rd_ost_lookup_vld;
    logic [RD_OST_SLOT_W-1:0] rd_ost_lookup_slot;
    ost_ctx_t rd_ost_lookup_ctx;
    logic rd_ost_empty;
    logic rd_ost_full;

    logic wr_ost_alloc_rdy;
    logic [WR_OST_SLOT_W-1:0] wr_ost_alloc_slot;
    logic wr_ost_lookup_vld;
    logic [WR_OST_SLOT_W-1:0] wr_ost_lookup_slot;
    ost_ctx_t wr_ost_lookup_ctx;
    logic wr_ost_empty;
    logic wr_ost_full;

    wire ar_hsk = axi4_ar_mst.vld && axi4_ar_mst.rdy;
    wire aw_hsk = axi4_aw_mst.vld && axi4_aw_mst.rdy;
    wire w_hsk = axi4_w_mst.vld && axi4_w_mst.rdy;
    wire r_hsk = axi4_r_slv.vld && axi4_r_slv.rdy;
    wire b_hsk = axi4_b_slv.vld && axi4_b_slv.rdy;
    wire rsp_sel_r = axi4_r_slv.vld && rd_ost_lookup_vld;
    wire rsp_sel_b = !rsp_sel_r && axi4_b_slv.vld && wr_ost_lookup_vld;

    function automatic logic req_aligned(
        input logic [31:0] addr,
        input bti_req_size_t size
    );
        unique case (size)
        BTI_REQ_SIZE_B1: req_aligned = 1'b1;
        BTI_REQ_SIZE_B2: req_aligned = addr[0] == 1'b0;
        BTI_REQ_SIZE_B4: req_aligned = addr[1:0] == 2'b00;
        default: req_aligned = 1'b0;
        endcase
    endfunction

    fifo #(
        .DW    (BTI_REQ_DW),
        .DEPTH (STG_FIFO_DEPTH)
    ) u_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (bti_req_slv.vld),
        .wr_rdy  (bti_req_slv.rdy),
        .wr_data (bti_req_slv.pkt),
        .rd_vld  (req_fifo_rd_vld),
        .rd_rdy  (req_fifo_rd_rdy),
        .rd_data (req_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    assign req_pkt = req_fifo_rd_data;
    assign req_is_read = req_pkt.cmd == BTI_REQ_CMD_READ;
    assign rd_issue_hsk = ar_hsk;
    assign wr_issue_hsk = (wr_aw_sent || aw_hsk) && (wr_w_sent || w_hsk);

    ostk #(
        .KEY_W (8),
        .DW    (OST_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_rd_ost(
        .clk         (clk),
        .rst_n       (rst_n),
        .alloc_vld   (rd_issue_hsk),
        .alloc_rdy   (rd_ost_alloc_rdy),
        .alloc_key   (8'h00),
        .alloc_ctx   (ost_ctx_t'{bti_trans_id:req_pkt.trans_id}),
        .alloc_slot  (rd_ost_alloc_slot),
        .lookup_key  (axi4_r_slv.pkt.id),
        .lookup_vld  (rd_ost_lookup_vld),
        .lookup_slot (rd_ost_lookup_slot),
        .lookup_ctx  (rd_ost_lookup_ctx),
        .free_vld    (r_hsk),
        .free_slot   (rd_ost_lookup_slot),
        .slot_wr_vld (1'b0),
        .slot_wr_idx ('0),
        .slot_wr_ctx ('0),
        .slot_vld    (),
        .slot_key_flat(),
        .slot_ctx_flat(),
        .slot_older_flat(),
        .empty       (rd_ost_empty),
        .full        (rd_ost_full)
    );

    ostk #(
        .KEY_W (8),
        .DW    (OST_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_wr_ost(
        .clk         (clk),
        .rst_n       (rst_n),
        .alloc_vld   (wr_issue_hsk),
        .alloc_rdy   (wr_ost_alloc_rdy),
        .alloc_key   (8'h00),
        .alloc_ctx   (ost_ctx_t'{bti_trans_id:req_pkt.trans_id}),
        .alloc_slot  (wr_ost_alloc_slot),
        .lookup_key  (axi4_b_slv.pkt.id),
        .lookup_vld  (wr_ost_lookup_vld),
        .lookup_slot (wr_ost_lookup_slot),
        .lookup_ctx  (wr_ost_lookup_ctx),
        .free_vld    (b_hsk),
        .free_slot   (wr_ost_lookup_slot),
        .slot_wr_vld (1'b0),
        .slot_wr_idx ('0),
        .slot_wr_ctx ('0),
        .slot_vld    (),
        .slot_key_flat(),
        .slot_ctx_flat(),
        .slot_older_flat(),
        .empty       (wr_ost_empty),
        .full        (wr_ost_full)
    );

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_aw_sent <= 1'b0;
            wr_w_sent <= 1'b0;
        end else begin
            if (!req_fifo_rd_vld || req_is_read || wr_issue_hsk) begin
                wr_aw_sent <= 1'b0;
                wr_w_sent <= 1'b0;
            end else begin
                if (aw_hsk)
                    wr_aw_sent <= 1'b1;
                if (w_hsk)
                    wr_w_sent <= 1'b1;
            end
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && bti_req_slv.vld && bti_req_slv.rdy) begin
            assert (req_aligned(bti_req_slv.pkt.addr, bti_req_slv.pkt.size))
                else $fatal(1, "bti2axi requires aligned BTI request");
        end
        if (rst_n && axi4_r_slv.vld) begin
            assert (rd_ost_lookup_vld)
                else $fatal(1, "bti2axi read response has no OST entry");
            assert (axi4_r_slv.pkt.last)
                else $fatal(1, "bti2axi expects single-beat read response");
        end
        if (rst_n && axi4_b_slv.vld) begin
            assert (wr_ost_lookup_vld)
                else $fatal(1, "bti2axi write response has no OST entry");
        end
    end
`endif

    assign req_fifo_rd_rdy =
        req_fifo_rd_vld &&
        ((req_is_read && rd_ost_alloc_rdy && axi4_ar_mst.rdy) ||
        (!req_is_read && wr_ost_alloc_rdy && wr_issue_hsk));

    assign axi4_ar_mst.vld = req_fifo_rd_vld && req_is_read &&
        rd_ost_alloc_rdy;
    assign axi4_ar_mst.pkt.id = 8'h00;
    assign axi4_ar_mst.pkt.addr = req_pkt.addr;
    assign axi4_ar_mst.pkt.len = 8'h00;
    assign axi4_ar_mst.pkt.size = axi4_ar_size_t'(req_pkt.size);
    assign axi4_ar_mst.pkt.burst = AXI4_AR_BURST_INCR;
    assign axi4_ar_mst.pkt.lock = 1'b0;
    assign axi4_ar_mst.pkt.cache = 4'h0;
    assign axi4_ar_mst.pkt.prot = 3'h0;
    assign axi4_ar_mst.pkt.qos = 4'h0;
    assign axi4_ar_mst.pkt.user = 32'h0;

    assign axi4_aw_mst.vld = req_fifo_rd_vld && !req_is_read &&
        wr_ost_alloc_rdy && !wr_aw_sent;
    assign axi4_aw_mst.pkt.id = 8'h00;
    assign axi4_aw_mst.pkt.addr = req_pkt.addr;
    assign axi4_aw_mst.pkt.len = 8'h00;
    assign axi4_aw_mst.pkt.size = axi4_aw_size_t'(req_pkt.size);
    assign axi4_aw_mst.pkt.burst = AXI4_AW_BURST_INCR;
    assign axi4_aw_mst.pkt.lock = 1'b0;
    assign axi4_aw_mst.pkt.cache = 4'h0;
    assign axi4_aw_mst.pkt.prot = 3'h0;
    assign axi4_aw_mst.pkt.qos = 4'h0;
    assign axi4_aw_mst.pkt.user = 32'h0;

    assign axi4_w_mst.vld = req_fifo_rd_vld && !req_is_read &&
        wr_ost_alloc_rdy && !wr_w_sent;
    assign axi4_w_mst.pkt.data = req_pkt.data;
    assign axi4_w_mst.pkt.strb = req_pkt.strobe;
    assign axi4_w_mst.pkt.last = 1'b1;

    assign axi4_r_slv.rdy = bti_rsp_mst.rdy && rsp_sel_r;
    assign axi4_b_slv.rdy = bti_rsp_mst.rdy && rsp_sel_b;

    assign bti_rsp_mst.vld = rsp_sel_r || rsp_sel_b;
    assign bti_rsp_mst.pkt.trans_id = rsp_sel_r ?
        rd_ost_lookup_ctx.bti_trans_id : wr_ost_lookup_ctx.bti_trans_id;
    assign bti_rsp_mst.pkt.data = rsp_sel_r ? axi4_r_slv.pkt.data : 32'h0;
    assign bti_rsp_mst.pkt.ok = rsp_sel_r ?
        (axi4_r_slv.pkt.resp == AXI4_R_RESP_OKAY) :
        (axi4_b_slv.pkt.resp == AXI4_B_RESP_OKAY);

    wire unused = (|rd_ost_alloc_slot) | (|wr_ost_alloc_slot) |
        rd_ost_empty | rd_ost_full | wr_ost_empty | wr_ost_full;
endmodule
