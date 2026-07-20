`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi_demux #(
    parameter int GST_NUM = 5,
    parameter logic [31:0] GST_BASE[GST_NUM],
    parameter logic [31:0] GST_SIZE[GST_NUM],
    parameter int STG_FIFO_DEPTH = 4,
    parameter int OST_DEPTH = 8,
    localparam int GST_IDX_W = GST_NUM <= 1 ? 1 : $clog2(GST_NUM),
    localparam int OST_SLOT_W = $clog2(OST_DEPTH)
)(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  host_axi4_aw_slv,
    axi4_w_if_t.slv   host_axi4_w_slv,
    axi4_b_if_t.mst   host_axi4_b_mst,
    axi4_ar_if_t.slv  host_axi4_ar_slv,
    axi4_r_if_t.mst   host_axi4_r_mst,
    axi4_aw_if_t.mst  gst_axi4_aw_msts[GST_NUM],
    axi4_w_if_t.mst   gst_axi4_w_msts[GST_NUM],
    axi4_b_if_t.slv   gst_axi4_b_slvs[GST_NUM],
    axi4_ar_if_t.mst  gst_axi4_ar_msts[GST_NUM],
    axi4_r_if_t.slv   gst_axi4_r_slvs[GST_NUM],
    perf_axi_demux_if_t.mst perf_mst
);
    typedef struct packed {
        logic [7:0] id;
        logic [31:0] addr;
        logic [7:0] len;
        axi4_ar_size_t size;
        axi4_ar_burst_t burst;
        logic lock;
        logic [3:0] cache;
        logic [2:0] prot;
        logic [3:0] qos;
        logic [31:0] user;
    } axi4_ar_pkt_t;

    typedef struct packed {
        logic [7:0] id;
        logic [31:0] addr;
        logic [7:0] len;
        axi4_aw_size_t size;
        axi4_aw_burst_t burst;
        logic lock;
        logic [3:0] cache;
        logic [2:0] prot;
        logic [3:0] qos;
        logic [31:0] user;
    } axi4_aw_pkt_t;

    typedef struct packed {
        logic [31:0] data;
        logic [3:0] strb;
        logic last;
    } axi4_w_pkt_t;

    typedef struct packed {
        logic [7:0] id;
        axi4_b_resp_t resp;
    } axi4_b_pkt_t;

    typedef struct packed {
        logic [7:0] id;
        logic [31:0] data;
        axi4_r_resp_t resp;
        logic last;
    } axi4_r_pkt_t;

    typedef struct packed {
        axi4_r_pkt_t pkt;
        logic free_vld;
        logic [OST_SLOT_W-1:0] free_slot;
        logic from_gst;
        logic [GST_IDX_W-1:0] gst_idx;
    } axi4_r_meta_t;

    typedef struct packed {
        axi4_b_pkt_t pkt;
        logic free_vld;
        logic [OST_SLOT_W-1:0] free_slot;
        logic from_gst;
        logic [GST_IDX_W-1:0] gst_idx;
    } axi4_b_meta_t;

    typedef struct packed {
        logic [GST_IDX_W-1:0] gst_idx;
    } rd_ctx_t;

    typedef struct packed {
        logic [GST_IDX_W-1:0] gst_idx;
        logic [OST_SLOT_W-1:0] rsp_slot;
        logic decerr;
    } wr_data_ctx_t;

    typedef struct packed {
        logic [GST_IDX_W-1:0] gst_idx;
        logic decerr;
        logic decerr_valid;
    } wr_rsp_ctx_t;

    localparam int AR_DW = $bits(axi4_ar_pkt_t);
    localparam int AW_DW = $bits(axi4_aw_pkt_t);
    localparam int W_DW = $bits(axi4_w_pkt_t);
    localparam int B_DW = $bits(axi4_b_pkt_t);
    localparam int R_DW = $bits(axi4_r_pkt_t);
    localparam int B_META_DW = $bits(axi4_b_meta_t);
    localparam int R_META_DW = $bits(axi4_r_meta_t);
    localparam int RD_CTX_DW = $bits(rd_ctx_t);
    localparam int WR_DATA_CTX_DW = $bits(wr_data_ctx_t);
    localparam int WR_RSP_CTX_DW = $bits(wr_rsp_ctx_t);

    logic [GST_NUM-1:0] gst_ar_rdy;
    logic [GST_NUM-1:0] gst_aw_rdy;
    logic [GST_NUM-1:0] gst_w_rdy;
    logic [GST_NUM-1:0] gst_r_vld;
    logic [GST_NUM-1:0] gst_r_rdy;
    logic [R_DW-1:0] gst_r_pkt_data[GST_NUM];
    axi4_r_pkt_t gst_r_pkt[GST_NUM];
    logic [7:0] gst_r_id[GST_NUM];
    logic [31:0] gst_r_data[GST_NUM];
    axi4_r_resp_t gst_r_resp[GST_NUM];
    logic [GST_NUM-1:0] gst_r_last;
    logic [GST_NUM-1:0] gst_b_vld;
    logic [GST_NUM-1:0] gst_b_rdy;
    logic [B_DW-1:0] gst_b_pkt_data[GST_NUM];
    axi4_b_pkt_t gst_b_pkt[GST_NUM];
    logic [7:0] gst_b_id[GST_NUM];
    axi4_b_resp_t gst_b_resp[GST_NUM];

    logic ar_fifo_rd_vld;
    logic ar_fifo_rd_rdy;
    logic [AR_DW-1:0] ar_fifo_rd_data;
    axi4_ar_pkt_t ar_pkt;
    logic aw_fifo_rd_vld;
    logic aw_fifo_rd_rdy;
    logic [AW_DW-1:0] aw_fifo_rd_data;
    axi4_aw_pkt_t aw_pkt;
    logic w_fifo_rd_vld;
    logic w_fifo_rd_rdy;
    logic [W_DW-1:0] w_fifo_rd_data;
    axi4_w_pkt_t w_pkt;

    logic ar_hit;
    logic [GST_IDX_W-1:0] ar_gst_idx;
    logic aw_hit;
    logic [GST_IDX_W-1:0] aw_gst_idx;

    logic [GST_NUM-1:0] ar_issue_vld;
    logic [GST_NUM-1:0] aw_issue_vld;
    logic [GST_NUM-1:0] w_issue_vld;

    logic rd_ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] rd_ost_alloc_slot;
    logic [OST_DEPTH-1:0] rd_ost_slot_vld;
    logic [OST_DEPTH*8-1:0] rd_ost_key_flat;
    logic [OST_DEPTH*RD_CTX_DW-1:0] rd_ost_ctx_flat;
    logic [OST_DEPTH*OST_DEPTH-1:0] rd_ost_older_flat;
    rd_ctx_t rd_slot_ctx[OST_DEPTH];
    logic [7:0] rd_slot_key[OST_DEPTH];

    logic wr_data_ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] wr_data_ost_alloc_slot;
    logic wr_data_head_vld;
    wr_data_ctx_t wr_data_head_ctx;
    logic [OST_SLOT_W-1:0] wr_data_head_slot;

    logic wr_rsp_ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] wr_rsp_ost_alloc_slot;
    logic wr_rsp_slot_wr_vld;
    logic [OST_SLOT_W-1:0] wr_rsp_slot_wr_idx;
    wr_rsp_ctx_t wr_rsp_slot_wr_ctx;
    logic [OST_DEPTH-1:0] wr_rsp_ost_slot_vld;
    logic [OST_DEPTH*8-1:0] wr_rsp_ost_key_flat;
    logic [OST_DEPTH*WR_RSP_CTX_DW-1:0] wr_rsp_ost_ctx_flat;
    logic [OST_DEPTH*OST_DEPTH-1:0] wr_rsp_ost_older_flat;
    wr_rsp_ctx_t wr_rsp_slot_ctx[OST_DEPTH];
    logic [7:0] wr_rsp_slot_key[OST_DEPTH];

    logic [GST_IDX_W-1:0] rd_rsp_rr_idx;
    logic [GST_IDX_W-1:0] wr_rsp_rr_idx;
    logic [GST_NUM-1:0] rd_rsp_match_vld;
    logic [OST_SLOT_W-1:0] rd_rsp_match_slot[GST_NUM];
    logic [OST_DEPTH-1:0] rd_rsp_match_candidate[GST_NUM];
    logic rd_rsp_sel_vld;
    logic [GST_IDX_W-1:0] rd_rsp_sel_idx;
    logic [OST_SLOT_W-1:0] rd_rsp_sel_slot;
    logic [GST_NUM-1:0] wr_rsp_match_vld;
    logic [OST_SLOT_W-1:0] wr_rsp_match_slot[GST_NUM];
    logic [OST_DEPTH-1:0] wr_rsp_match_candidate[GST_NUM];
    logic wr_rsp_sel_vld;
    logic [GST_IDX_W-1:0] wr_rsp_sel_idx;
    logic [OST_SLOT_W-1:0] wr_rsp_sel_slot;
    logic rd_free_vld;
    logic [OST_SLOT_W-1:0] rd_free_slot;
    logic wr_free_vld;
    logic [OST_SLOT_W-1:0] wr_free_slot;
    logic rd_free_pending;
    logic wr_free_pending;
    logic decerr_b_sel_vld;
    logic [OST_SLOT_W-1:0] decerr_b_sel_slot;
    logic [OST_DEPTH-1:0] decerr_b_candidate_pre_vld;
    logic [OST_DEPTH-1:0] decerr_b_candidate_q_vld;
    logic [OST_DEPTH-1:0] decerr_b_candidate_vld;
    logic [OST_DEPTH-1:0] decerr_b_older_same_key[OST_DEPTH];
    logic ar_decerr_sel;
    logic b_sel_decerr;
    logic pre_host_r_vld;
    logic pre_host_r_rdy;
    axi4_r_pkt_t pre_host_r_pkt;
    axi4_r_meta_t pre_host_r_meta;
    logic host_r_slice_rdy;
    logic rd_rsp_stage_vld;
    axi4_r_meta_t rd_rsp_stage_meta;
    logic [R_META_DW-1:0] rd_rsp_stage_data;
    axi4_r_meta_t host_r_meta;
    logic [R_META_DW-1:0] pre_host_r_data;
    logic [R_META_DW-1:0] host_r_data;
    logic pre_host_b_vld;
    logic pre_host_b_rdy;
    axi4_b_pkt_t pre_host_b_pkt;
    axi4_b_meta_t pre_host_b_meta;
    logic host_b_slice_rdy;
    logic wr_rsp_stage_vld;
    axi4_b_meta_t wr_rsp_stage_meta;
    logic [B_META_DW-1:0] wr_rsp_stage_data;
    axi4_b_meta_t host_b_meta;
    logic [B_META_DW-1:0] pre_host_b_data;
    logic [B_META_DW-1:0] host_b_data;

    wire ar_hsk = ar_fifo_rd_vld && ar_fifo_rd_rdy;
    wire aw_hsk = aw_fifo_rd_vld && aw_fifo_rd_rdy;
    wire w_hsk = w_fifo_rd_vld && w_fifo_rd_rdy;
    wire ar_issue_hsk = ar_fifo_rd_vld && ar_hit &&
        rd_ost_alloc_rdy && gst_ar_rdy[ar_gst_idx];
    wire rd_rsp_hsk = rd_rsp_stage_vld && host_r_slice_rdy;
    wire wr_rsp_hsk = wr_rsp_stage_vld && host_b_slice_rdy;

    function automatic logic addr_in_range(
        input logic [31:0] addr,
        input logic [31:0] base,
        input logic [31:0] size
    );
        addr_in_range = size != 32'h0 &&
            ({1'b0, addr} - {1'b0, base}) < {1'b0, size};
    endfunction

    function automatic logic [GST_IDX_W-1:0] gst_idx(input int unsigned idx);
        gst_idx = idx[GST_IDX_W-1:0];
    endfunction

    fifo #(
        .DW           (AR_DW),
        .DEPTH        (STG_FIFO_DEPTH),
        .FALL_THROUGH (1'b1)
    ) u_ar_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_axi4_ar_slv.vld),
        .wr_rdy  (host_axi4_ar_slv.rdy),
        .wr_data (host_axi4_ar_slv.pkt),
        .rd_vld  (ar_fifo_rd_vld),
        .rd_rdy  (ar_fifo_rd_rdy),
        .rd_data (ar_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW           (AW_DW),
        .DEPTH        (STG_FIFO_DEPTH),
        .FALL_THROUGH (1'b1)
    ) u_aw_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_axi4_aw_slv.vld),
        .wr_rdy  (host_axi4_aw_slv.rdy),
        .wr_data (host_axi4_aw_slv.pkt),
        .rd_vld  (aw_fifo_rd_vld),
        .rd_rdy  (aw_fifo_rd_rdy),
        .rd_data (aw_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    fifo #(
        .DW           (W_DW),
        .DEPTH        (STG_FIFO_DEPTH),
        .FALL_THROUGH (1'b1)
    ) u_w_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_axi4_w_slv.vld),
        .wr_rdy  (host_axi4_w_slv.rdy),
        .wr_data (host_axi4_w_slv.pkt),
        .rd_vld  (w_fifo_rd_vld),
        .rd_rdy  (w_fifo_rd_rdy),
        .rd_data (w_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    assign ar_pkt = ar_fifo_rd_data;
    assign aw_pkt = aw_fifo_rd_data;
    assign w_pkt = w_fifo_rd_data;

    ostk #(
        .KEY_W (8),
        .DW    (RD_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_rd_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (ar_issue_hsk),
        .alloc_rdy    (rd_ost_alloc_rdy),
        .alloc_key    (ar_pkt.id),
        .alloc_ctx    (rd_ctx_t'{gst_idx:ar_gst_idx}),
        .alloc_slot   (rd_ost_alloc_slot),
        .lookup_key   ('0),
        .lookup_vld   (),
        .lookup_slot  (),
        .lookup_ctx   (),
        .free_vld     (rd_free_vld),
        .free_slot    (rd_free_slot),
        .slot_wr_vld  (1'b0),
        .slot_wr_idx  ('0),
        .slot_wr_ctx  ('0),
        .slot_vld     (rd_ost_slot_vld),
        .slot_key_flat(rd_ost_key_flat),
        .slot_ctx_flat(rd_ost_ctx_flat),
        .slot_older_flat(rd_ost_older_flat),
        .empty        (),
        .full         ()
    );

    ostq #(
        .DW    (WR_DATA_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_wr_data_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (aw_hsk),
        .alloc_rdy    (wr_data_ost_alloc_rdy),
        .alloc_ctx    (wr_data_ctx_t'{gst_idx:aw_gst_idx,
            rsp_slot:wr_rsp_ost_alloc_slot, decerr:!aw_hit}),
        .alloc_slot   (wr_data_ost_alloc_slot),
        .head_vld     (wr_data_head_vld),
        .head_ctx     (wr_data_head_ctx),
        .head_slot    (wr_data_head_slot),
        .free_head    (w_hsk && w_pkt.last),
        .slot_wr_vld  (1'b0),
        .slot_wr_idx  ('0),
        .slot_wr_ctx  ('0),
        .slot_vld     (),
        .slot_ctx_flat(),
        .empty        (),
        .full         ()
    );

    ostk #(
        .KEY_W (8),
        .DW    (WR_RSP_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_wr_rsp_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (aw_hsk),
        .alloc_rdy    (wr_rsp_ost_alloc_rdy),
        .alloc_key    (aw_pkt.id),
        .alloc_ctx    (wr_rsp_ctx_t'{gst_idx:aw_gst_idx,
            decerr:!aw_hit, decerr_valid:1'b0}),
        .alloc_slot   (wr_rsp_ost_alloc_slot),
        .lookup_key   ('0),
        .lookup_vld   (),
        .lookup_slot  (),
        .lookup_ctx   (),
        .free_vld     (wr_free_vld),
        .free_slot    (wr_free_slot),
        .slot_wr_vld  (wr_rsp_slot_wr_vld),
        .slot_wr_idx  (wr_rsp_slot_wr_idx),
        .slot_wr_ctx  (wr_rsp_slot_wr_ctx),
        .slot_vld     (wr_rsp_ost_slot_vld),
        .slot_key_flat(wr_rsp_ost_key_flat),
        .slot_ctx_flat(wr_rsp_ost_ctx_flat),
        .slot_older_flat(wr_rsp_ost_older_flat),
        .empty        (),
        .full         ()
    );

    for (genvar i = 0; i < OST_DEPTH; i++) begin : gen_slot_view
        assign rd_slot_key[i] = rd_ost_key_flat[i * 8 +: 8];
        assign rd_slot_ctx[i] =
            rd_ost_ctx_flat[i * RD_CTX_DW +: RD_CTX_DW];
        assign wr_rsp_slot_key[i] = wr_rsp_ost_key_flat[i * 8 +: 8];
        assign wr_rsp_slot_ctx[i] =
            wr_rsp_ost_ctx_flat[i * WR_RSP_CTX_DW +: WR_RSP_CTX_DW];
        for (genvar j = 0; j < OST_DEPTH; j++) begin : gen_decerr_older
            assign decerr_b_older_same_key[i][j] =
                wr_rsp_ost_slot_vld[j] &&
                wr_rsp_slot_key[j] == wr_rsp_slot_key[i] &&
                wr_rsp_ost_older_flat[j * OST_DEPTH + i];
        end

        assign decerr_b_candidate_pre_vld[i] = wr_rsp_ost_slot_vld[i] &&
            wr_rsp_slot_ctx[i].decerr &&
            wr_rsp_slot_ctx[i].decerr_valid &&
            !(|decerr_b_older_same_key[i]);
        assign decerr_b_candidate_vld[i] = decerr_b_candidate_q_vld[i] &&
            wr_rsp_ost_slot_vld[i] &&
            !(wr_free_pending && wr_rsp_stage_meta.free_slot == OST_SLOT_W'(i));
    end

    assign rd_free_pending = rd_rsp_stage_vld &&
        rd_rsp_stage_meta.free_vld;
    assign wr_free_pending = wr_rsp_stage_vld &&
        wr_rsp_stage_meta.free_vld;

    for (genvar i = 0; i < GST_NUM; i++) begin : gen_rsp_match
        for (genvar s = 0; s < OST_DEPTH; s++) begin : gen_candidate
            assign rd_rsp_match_candidate[i][s] = rd_ost_slot_vld[s] &&
                rd_slot_key[s] == gst_r_id[i] &&
                !(rd_free_pending &&
                rd_rsp_stage_meta.free_slot == OST_SLOT_W'(s));
            assign wr_rsp_match_candidate[i][s] =
                wr_rsp_ost_slot_vld[s] &&
                wr_rsp_slot_key[s] == gst_b_id[i] &&
                !(wr_free_pending &&
                wr_rsp_stage_meta.free_slot == OST_SLOT_W'(s));
        end

        oldest_select #(
            .DEPTH (OST_DEPTH)
        ) u_rd_rsp_match(
            .candidate_vld (rd_rsp_match_candidate[i]),
            .older_flat    (rd_ost_older_flat),
            .select_vld    (rd_rsp_match_vld[i]),
            .select_slot   (rd_rsp_match_slot[i])
        );

        oldest_select #(
            .DEPTH (OST_DEPTH)
        ) u_wr_rsp_match(
            .candidate_vld (wr_rsp_match_candidate[i]),
            .older_flat    (wr_rsp_ost_older_flat),
            .select_vld    (wr_rsp_match_vld[i]),
            .select_slot   (wr_rsp_match_slot[i])
        );
    end

    oldest_select #(
        .DEPTH (OST_DEPTH)
    ) u_decerr_b_select(
        .candidate_vld (decerr_b_candidate_vld),
        .older_flat    (wr_rsp_ost_older_flat),
        .select_vld    (decerr_b_sel_vld),
        .select_slot   (decerr_b_sel_slot)
    );

    always_comb begin
        ar_hit = 1'b0;
        ar_gst_idx = '0;
        for (int unsigned i = 0; i < GST_NUM; i++) begin
            if (!ar_hit && addr_in_range(ar_pkt.addr, GST_BASE[i], GST_SIZE[i])) begin
                ar_hit = 1'b1;
                ar_gst_idx = gst_idx(i);
            end
        end
    end

    always_comb begin
        aw_hit = 1'b0;
        aw_gst_idx = '0;
        for (int unsigned i = 0; i < GST_NUM; i++) begin
            if (!aw_hit && addr_in_range(aw_pkt.addr, GST_BASE[i], GST_SIZE[i])) begin
                aw_hit = 1'b1;
                aw_gst_idx = gst_idx(i);
            end
        end
    end

    always_comb begin
        rd_rsp_sel_vld = 1'b0;
        rd_rsp_sel_idx = '0;
        rd_rsp_sel_slot = '0;
        for (int unsigned off = 0; off < GST_NUM; off++) begin
            int unsigned gi;
            gi = (int'(rd_rsp_rr_idx) + off) % GST_NUM;
            if (!rd_rsp_sel_vld && gst_r_vld[gi] &&
                rd_rsp_match_vld[gi] &&
                rd_slot_ctx[rd_rsp_match_slot[gi]].gst_idx == gst_idx(gi)) begin
                rd_rsp_sel_vld = 1'b1;
                rd_rsp_sel_idx = gst_idx(gi);
                rd_rsp_sel_slot = rd_rsp_match_slot[gi];
            end
        end
    end

    always_comb begin
        wr_rsp_sel_vld = 1'b0;
        wr_rsp_sel_idx = '0;
        wr_rsp_sel_slot = '0;
        for (int unsigned off = 0; off < GST_NUM; off++) begin
            int unsigned gi;
            gi = (int'(wr_rsp_rr_idx) + off) % GST_NUM;
            if (!wr_rsp_sel_vld && gst_b_vld[gi] &&
                wr_rsp_match_vld[gi] &&
                !wr_rsp_slot_ctx[wr_rsp_match_slot[gi]].decerr &&
                wr_rsp_slot_ctx[wr_rsp_match_slot[gi]].gst_idx == gst_idx(gi)) begin
                wr_rsp_sel_vld = 1'b1;
                wr_rsp_sel_idx = gst_idx(gi);
                wr_rsp_sel_slot = wr_rsp_match_slot[gi];
            end
        end
    end

    always_comb begin
        ar_issue_vld = '0;
        aw_issue_vld = '0;
        w_issue_vld = '0;
        if (ar_issue_hsk)
            ar_issue_vld[ar_gst_idx] = 1'b1;
        if (aw_fifo_rd_vld && wr_data_ost_alloc_rdy &&
            wr_rsp_ost_alloc_rdy && aw_hit)
            aw_issue_vld[aw_gst_idx] = 1'b1;
        if (w_fifo_rd_vld && wr_data_head_vld && !wr_data_head_ctx.decerr)
            w_issue_vld[wr_data_head_ctx.gst_idx] = 1'b1;
    end

    always_comb begin
        wr_rsp_slot_wr_vld = 1'b0;
        wr_rsp_slot_wr_idx = wr_data_head_ctx.rsp_slot;
        wr_rsp_slot_wr_ctx = '0;
        if (w_hsk && w_pkt.last && wr_data_head_ctx.decerr) begin
            wr_rsp_slot_wr_vld = 1'b1;
            wr_rsp_slot_wr_ctx = wr_rsp_slot_ctx[wr_data_head_ctx.rsp_slot];
            wr_rsp_slot_wr_ctx.decerr_valid = 1'b1;
        end
    end

    assign ar_decerr_sel = ar_fifo_rd_vld && !ar_hit && !rd_rsp_sel_vld;
    assign ar_fifo_rd_rdy = ar_hit ?
        (rd_ost_alloc_rdy && gst_ar_rdy[ar_gst_idx]) :
        (!rd_rsp_sel_vld && pre_host_r_rdy);
    assign aw_fifo_rd_rdy = aw_hit ?
        (wr_data_ost_alloc_rdy && wr_rsp_ost_alloc_rdy &&
            gst_aw_rdy[aw_gst_idx]) :
        (wr_data_ost_alloc_rdy && wr_rsp_ost_alloc_rdy);
    assign w_fifo_rd_rdy = w_fifo_rd_vld && wr_data_head_vld &&
        (wr_data_head_ctx.decerr || gst_w_rdy[wr_data_head_ctx.gst_idx]);

    assign b_sel_decerr = decerr_b_sel_vld;

    assign perf_mst.pkt.rd_ost_full = ar_fifo_rd_vld && ar_hit &&
        !rd_ost_alloc_rdy;
    assign perf_mst.pkt.wr_ost_full = aw_fifo_rd_vld &&
        (!wr_data_ost_alloc_rdy || !wr_rsp_ost_alloc_rdy);
    assign perf_mst.pkt.ar_stg_full = host_axi4_ar_slv.vld &&
        !host_axi4_ar_slv.rdy;
    assign perf_mst.pkt.aw_stg_full = host_axi4_aw_slv.vld &&
        !host_axi4_aw_slv.rdy;
    assign perf_mst.pkt.w_stg_full = host_axi4_w_slv.vld &&
        !host_axi4_w_slv.rdy;
    assign perf_mst.pkt.ar_issue_bp = ar_fifo_rd_vld &&
        !ar_fifo_rd_rdy;
    assign perf_mst.pkt.aw_issue_bp = aw_fifo_rd_vld &&
        !aw_fifo_rd_rdy;
    assign perf_mst.pkt.w_issue_bp = w_fifo_rd_vld &&
        !w_fifo_rd_rdy;
    assign perf_mst.pkt.r_rsp_bp = host_axi4_r_mst.vld &&
        !host_axi4_r_mst.rdy;
    assign perf_mst.pkt.b_rsp_bp = host_axi4_b_mst.vld &&
        !host_axi4_b_mst.rdy;

    for (genvar i = 0; i < GST_NUM; i++) begin : gen_gst
        assign gst_ar_rdy[i] = gst_axi4_ar_msts[i].rdy;
        assign gst_aw_rdy[i] = gst_axi4_aw_msts[i].rdy;
        assign gst_w_rdy[i] = gst_axi4_w_msts[i].rdy;

        rdy_reg_slice #(
            .DW (R_DW)
        ) u_r_reg_slice(
            .clk      (clk),
            .rst_n    (rst_n),
            .clear    (1'b0),
            .src_vld  (gst_axi4_r_slvs[i].vld),
            .src_rdy  (gst_axi4_r_slvs[i].rdy),
            .src_data (gst_axi4_r_slvs[i].pkt),
            .dst_vld  (gst_r_vld[i]),
            .dst_rdy  (gst_r_rdy[i]),
            .dst_data (gst_r_pkt_data[i])
        );

        rdy_reg_slice #(
            .DW (B_DW)
        ) u_b_reg_slice(
            .clk      (clk),
            .rst_n    (rst_n),
            .clear    (1'b0),
            .src_vld  (gst_axi4_b_slvs[i].vld),
            .src_rdy  (gst_axi4_b_slvs[i].rdy),
            .src_data (gst_axi4_b_slvs[i].pkt),
            .dst_vld  (gst_b_vld[i]),
            .dst_rdy  (gst_b_rdy[i]),
            .dst_data (gst_b_pkt_data[i])
        );

        assign gst_r_pkt[i] = axi4_r_pkt_t'(gst_r_pkt_data[i]);
        assign gst_r_id[i] = gst_r_pkt[i].id;
        assign gst_r_data[i] = gst_r_pkt[i].data;
        assign gst_r_resp[i] = gst_r_pkt[i].resp;
        assign gst_r_last[i] = gst_r_pkt[i].last;
        assign gst_b_pkt[i] = axi4_b_pkt_t'(gst_b_pkt_data[i]);
        assign gst_b_id[i] = gst_b_pkt[i].id;
        assign gst_b_resp[i] = gst_b_pkt[i].resp;

        assign gst_axi4_ar_msts[i].vld = ar_issue_vld[i];
        assign gst_axi4_ar_msts[i].pkt = ar_pkt;
        assign gst_axi4_aw_msts[i].vld = aw_issue_vld[i];
        assign gst_axi4_aw_msts[i].pkt = aw_pkt;
        assign gst_axi4_w_msts[i].vld = w_issue_vld[i];
        assign gst_axi4_w_msts[i].pkt = w_pkt;
        assign gst_r_rdy[i] = rd_rsp_sel_vld &&
            rd_rsp_sel_idx == gst_idx(i) && pre_host_r_rdy;
        assign gst_b_rdy[i] = !b_sel_decerr && wr_rsp_sel_vld &&
            wr_rsp_sel_idx == gst_idx(i) && pre_host_b_rdy;
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_rsp_rr_idx <= '0;
            wr_rsp_rr_idx <= '0;
            decerr_b_candidate_q_vld <= '0;
        end else begin
            decerr_b_candidate_q_vld <= decerr_b_candidate_pre_vld;
            if (rd_rsp_hsk && rd_rsp_stage_meta.from_gst)
                rd_rsp_rr_idx <= rd_rsp_stage_meta.gst_idx + 1'b1;
            if (wr_rsp_hsk && wr_rsp_stage_meta.from_gst)
                wr_rsp_rr_idx <= wr_rsp_stage_meta.gst_idx + 1'b1;
        end
    end

    assign rd_free_vld = rd_rsp_hsk && rd_rsp_stage_meta.free_vld;
    assign rd_free_slot = rd_rsp_stage_meta.free_slot;
    assign wr_free_vld = wr_rsp_hsk && wr_rsp_stage_meta.free_vld;
    assign wr_free_slot = wr_rsp_stage_meta.free_slot;

    assign pre_host_r_vld = rd_rsp_sel_vld || ar_decerr_sel;
    assign pre_host_r_pkt.id = ar_decerr_sel ? ar_pkt.id :
        gst_r_id[rd_rsp_sel_idx];
    assign pre_host_r_pkt.data = ar_decerr_sel ? 32'h0 :
        gst_r_data[rd_rsp_sel_idx];
    assign pre_host_r_pkt.resp = ar_decerr_sel ? AXI4_R_RESP_DECERR :
        gst_r_resp[rd_rsp_sel_idx];
    assign pre_host_r_pkt.last = ar_decerr_sel ? 1'b1 :
        gst_r_last[rd_rsp_sel_idx];
    assign pre_host_r_meta.pkt = pre_host_r_pkt;
    assign pre_host_r_meta.free_vld = rd_rsp_sel_vld &&
        gst_r_last[rd_rsp_sel_idx];
    assign pre_host_r_meta.free_slot = rd_rsp_sel_slot;
    assign pre_host_r_meta.from_gst = rd_rsp_sel_vld;
    assign pre_host_r_meta.gst_idx = rd_rsp_sel_idx;
    assign pre_host_r_data = pre_host_r_meta;
    assign pre_host_r_rdy = !rd_rsp_stage_vld;
    assign rd_rsp_stage_data = rd_rsp_stage_meta;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rd_rsp_stage_vld <= 1'b0;
            rd_rsp_stage_meta <= '0;
        end else begin
            if (rd_rsp_hsk)
                rd_rsp_stage_vld <= 1'b0;
            if (!rd_rsp_stage_vld && pre_host_r_vld) begin
                rd_rsp_stage_vld <= 1'b1;
                rd_rsp_stage_meta <= pre_host_r_meta;
            end
        end
    end

    vld_reg_slice #(
        .DW (R_META_DW)
    ) u_host_r_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (rd_rsp_stage_vld),
        .src_rdy  (host_r_slice_rdy),
        .src_data (rd_rsp_stage_data),
        .dst_vld  (host_axi4_r_mst.vld),
        .dst_rdy  (host_axi4_r_mst.rdy),
        .dst_data (host_r_data)
    );

    assign host_r_meta = axi4_r_meta_t'(host_r_data);
    assign host_axi4_r_mst.pkt = host_r_meta.pkt;

    assign pre_host_b_vld = b_sel_decerr || wr_rsp_sel_vld;
    assign pre_host_b_pkt.id = b_sel_decerr ?
        wr_rsp_slot_key[decerr_b_sel_slot] : gst_b_id[wr_rsp_sel_idx];
    assign pre_host_b_pkt.resp = b_sel_decerr ? AXI4_B_RESP_DECERR :
        gst_b_resp[wr_rsp_sel_idx];
    assign pre_host_b_meta.pkt = pre_host_b_pkt;
    assign pre_host_b_meta.free_vld = pre_host_b_vld;
    assign pre_host_b_meta.free_slot = b_sel_decerr ?
        decerr_b_sel_slot : wr_rsp_sel_slot;
    assign pre_host_b_meta.from_gst = wr_rsp_sel_vld && !b_sel_decerr;
    assign pre_host_b_meta.gst_idx = wr_rsp_sel_idx;
    assign pre_host_b_data = pre_host_b_meta;
    assign pre_host_b_rdy = !wr_rsp_stage_vld;
    assign wr_rsp_stage_data = wr_rsp_stage_meta;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_rsp_stage_vld <= 1'b0;
            wr_rsp_stage_meta <= '0;
        end else begin
            if (wr_rsp_hsk)
                wr_rsp_stage_vld <= 1'b0;
            if (!wr_rsp_stage_vld && pre_host_b_vld) begin
                wr_rsp_stage_vld <= 1'b1;
                wr_rsp_stage_meta <= pre_host_b_meta;
            end
        end
    end

    vld_reg_slice #(
        .DW (B_META_DW)
    ) u_host_b_reg_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (wr_rsp_stage_vld),
        .src_rdy  (host_b_slice_rdy),
        .src_data (wr_rsp_stage_data),
        .dst_vld  (host_axi4_b_mst.vld),
        .dst_rdy  (host_axi4_b_mst.rdy),
        .dst_data (host_b_data)
    );

    assign host_b_meta = axi4_b_meta_t'(host_b_data);
    assign host_axi4_b_mst.pkt = host_b_meta.pkt;

    wire unused = (|rd_ost_alloc_slot) | (|wr_data_ost_alloc_slot) |
        (|wr_data_head_slot);
endmodule
