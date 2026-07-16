module bti_mux2 #(
    parameter int STG_FIFO_DEPTH = 4,
    parameter int OST_DEPTH = 8
)(
    input logic clk,
    input logic rst_n,
    bti_req_if_t.slv host0_req_slv,
    bti_rsp_if_t.mst host0_rsp_mst,
    bti_req_if_t.slv host1_req_slv,
    bti_rsp_if_t.mst host1_rsp_mst,
    bti_req_if_t.mst gst_req_mst,
    bti_rsp_if_t.slv gst_rsp_slv
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
        logic [15:0] trans_id;
        logic [31:0] data;
        logic ok;
    } bti_rsp_pkt_t;

    localparam int BTI_REQ_DW = $bits(bti_req_pkt_t);
    localparam int OST_SLOT_W = $clog2(OST_DEPTH);

    logic rr;

    logic host0_fifo_rd_vld;
    logic host0_fifo_rd_rdy;
    logic [BTI_REQ_DW-1:0] host0_fifo_rd_data;
    logic [BTI_REQ_DW-1:0] host0_fifo_wr_data;
    bti_req_pkt_t host0_req_pkt;
    logic host1_fifo_rd_vld;
    logic host1_fifo_rd_rdy;
    logic [BTI_REQ_DW-1:0] host1_fifo_rd_data;
    logic [BTI_REQ_DW-1:0] host1_fifo_wr_data;
    bti_req_pkt_t host1_req_pkt;

    logic select1;
    bti_req_pkt_t issue_req_pkt;
    logic req_hsk;
    wire gst_rsp_hsk = gst_rsp_slv.vld && gst_rsp_slv.rdy;
    wire host_rsp_hsk = (host0_rsp_mst.vld && host0_rsp_mst.rdy) ||
        (host1_rsp_mst.vld && host1_rsp_mst.rdy);

    logic ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] ost_alloc_slot;
    logic ost_lookup_vld;
    logic [OST_SLOT_W-1:0] ost_lookup_slot;
    logic ost_lookup_ctx;
    logic ost_empty;
    logic ost_full;
    logic rsp_vld_q;
    logic rsp_ctx_q;
    bti_rsp_pkt_t rsp_pkt_q;

    assign host0_fifo_wr_data = host0_req_slv.pkt;
    assign host1_fifo_wr_data = host1_req_slv.pkt;
    assign host0_req_pkt = host0_fifo_rd_data;
    assign host1_req_pkt = host1_fifo_rd_data;
    assign select1 = host1_fifo_rd_vld && (rr || !host0_fifo_rd_vld);
    assign issue_req_pkt = select1 ? host1_req_pkt : host0_req_pkt;
    assign req_hsk = gst_req_mst.vld && gst_req_mst.rdy;

    fifo #(
        .DW    (BTI_REQ_DW),
        .DEPTH (STG_FIFO_DEPTH)
    ) u_host0_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host0_req_slv.vld),
        .wr_rdy  (host0_req_slv.rdy),
        .wr_data (host0_fifo_wr_data),
        .rd_vld  (host0_fifo_rd_vld),
        .rd_rdy  (host0_fifo_rd_rdy),
        .rd_data (host0_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW    (BTI_REQ_DW),
        .DEPTH (STG_FIFO_DEPTH)
    ) u_host1_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host1_req_slv.vld),
        .wr_rdy  (host1_req_slv.rdy),
        .wr_data (host1_fifo_wr_data),
        .rd_vld  (host1_fifo_rd_vld),
        .rd_rdy  (host1_fifo_rd_rdy),
        .rd_data (host1_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    ostk #(
        .KEY_W (16),
        .DW    (1),
        .DEPTH (OST_DEPTH)
    ) u_ost(
        .clk         (clk),
        .rst_n       (rst_n),
        .alloc_vld   (req_hsk),
        .alloc_rdy   (ost_alloc_rdy),
        .alloc_key   (issue_req_pkt.trans_id),
        .alloc_ctx   (select1),
        .alloc_slot  (ost_alloc_slot),
        .lookup_key  (gst_rsp_slv.pkt.trans_id),
        .lookup_vld  (ost_lookup_vld),
        .lookup_slot (ost_lookup_slot),
        .lookup_ctx  (ost_lookup_ctx),
        .free_vld    (gst_rsp_hsk),
        .free_slot   (ost_lookup_slot),
        .slot_wr_vld (1'b0),
        .slot_wr_idx ('0),
        .slot_wr_ctx ('0),
        .slot_vld    (),
        .slot_key_flat(),
        .slot_ctx_flat(),
        .slot_older_flat(),
        .empty       (ost_empty),
        .full        (ost_full)
    );

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rr <= 1'b0;
            rsp_vld_q <= 1'b0;
            rsp_ctx_q <= 1'b0;
            rsp_pkt_q <= '0;
        end else begin
            if (req_hsk)
                rr <= !select1;
            if (host_rsp_hsk)
                rsp_vld_q <= 1'b0;
            if (gst_rsp_hsk) begin
                rsp_vld_q <= 1'b1;
                rsp_ctx_q <= ost_lookup_ctx;
                rsp_pkt_q <= gst_rsp_slv.pkt;
            end
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && gst_req_mst.vld && gst_req_mst.rdy) begin
            assert (ost_alloc_rdy)
                else $fatal(1, "bti_mux2 request issued without OST space");
        end
        if (rst_n && gst_rsp_slv.vld) begin
            assert (ost_lookup_vld)
                else $fatal(1, "bti_mux2 response has no OST entry");
        end
    end
`endif

    assign gst_req_mst.vld = ost_alloc_rdy &&
        (host0_fifo_rd_vld || host1_fifo_rd_vld);
    assign gst_req_mst.pkt = issue_req_pkt;
    assign host0_fifo_rd_rdy = req_hsk && !select1;
    assign host1_fifo_rd_rdy = req_hsk && select1;

    assign host0_rsp_mst.vld = rsp_vld_q && !rsp_ctx_q;
    assign host0_rsp_mst.pkt = rsp_pkt_q;
    assign host1_rsp_mst.vld = rsp_vld_q && rsp_ctx_q;
    assign host1_rsp_mst.pkt = rsp_pkt_q;
    assign gst_rsp_slv.rdy = !rsp_vld_q && ost_lookup_vld;

    wire unused_ost = ost_empty | ost_full | (|ost_alloc_slot);
endmodule
