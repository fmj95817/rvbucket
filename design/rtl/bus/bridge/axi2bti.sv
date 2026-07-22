`include "itf/bti_req_if.svh"
`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"

module axi2bti #(
    parameter int STG_FIFO_DEPTH = 4,
    parameter int OST_DEPTH = 8,
    localparam int OST_SLOT_W = $clog2(OST_DEPTH)
)(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  axi4_aw_slv,
    axi4_w_if_t.slv   axi4_w_slv,
    axi4_b_if_t.mst   axi4_b_mst,
    axi4_ar_if_t.slv  axi4_ar_slv,
    axi4_r_if_t.mst   axi4_r_mst,
    bti_req_if_t.mst  bti_req_mst,
    bti_rsp_if_t.slv  bti_rsp_slv
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
        logic [7:0] axid;
        logic [31:0] axaddr;
        logic [7:0] axlen;
        logic [2:0] axsize;
        logic [1:0] axburst;
        logic [7:0] beat_idx;
        logic bti_rsp_pending;
        logic rsp_valid;
        logic rsp_ok;
        logic [31:0] rsp_data;
    } rd_ctx_t;

    typedef struct packed {
        logic [7:0] axid;
        logic [31:0] axaddr;
        logic [7:0] axlen;
        logic [2:0] axsize;
        logic [1:0] axburst;
        logic [7:0] beat_idx;
        logic bti_rsp_pending;
        logic pending_last;
        logic b_valid;
        logic b_ok;
    } wr_ctx_t;

    typedef enum logic {
        BEAT_READ,
        BEAT_WRITE
    } beat_kind_t;

    typedef struct packed {
        beat_kind_t kind;
        logic [OST_SLOT_W-1:0] slot;
        logic wr_last;
    } beat_ctx_t;

    typedef struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        bti_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } bti_req_pkt_t;

    localparam int AR_DW = $bits(axi4_ar_pkt_t);
    localparam int AW_DW = $bits(axi4_aw_pkt_t);
    localparam int W_DW = $bits(axi4_w_pkt_t);
    localparam int BEAT_CTX_DW = $bits(beat_ctx_t);

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

    logic rd_ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] rd_ost_alloc_slot;
    logic [OST_DEPTH-1:0] rd_ost_slot_vld;
    logic [OST_DEPTH*8-1:0] rd_ost_key_flat;
    logic [OST_DEPTH*OST_DEPTH-1:0] rd_ost_older_flat;
    logic [7:0] rd_ost_key[OST_DEPTH];
    rd_ctx_t rd_ctx_mem[OST_DEPTH];
    logic [OST_DEPTH-1:0] issue_rd_candidate_vld;
    logic issue_rd_select_vld;
    logic [OST_SLOT_W-1:0] issue_rd_select_slot;
    logic [OST_DEPTH-1:0] rd_out_candidate_vld;
    logic [OST_DEPTH-1:0] rd_out_older_same_id[OST_DEPTH];
    logic rd_out_select_vld;
    logic [OST_SLOT_W-1:0] rd_out_select_slot;
    logic rd_out_stage_vld;
    logic rd_out_stage_rdy;
    logic rd_out_stage_fill_hsk;
    logic [OST_SLOT_W-1:0] rd_out_stage_slot;
    rd_ctx_t rd_out_stage_ctx;

    logic wr_ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] wr_ost_alloc_slot;
    logic wr_ost_head_vld;
    logic [OST_SLOT_W-1:0] wr_ost_head_slot;
    logic [OST_DEPTH-1:0] wr_ost_slot_vld;
    logic wr_ost_empty;
    wr_ctx_t wr_ctx_mem[OST_DEPTH];

    logic beat_ost_alloc_rdy;
    logic [OST_SLOT_W-1:0] beat_ost_alloc_slot;
    logic beat_ost_lookup_vld;
    logic [OST_SLOT_W-1:0] beat_ost_lookup_slot;
    beat_ctx_t beat_ost_lookup_ctx;

    logic issue_wr_prio;
    logic [15:0] next_bti_trans_id;
    logic issue_sel_rd;
    logic issue_sel_wr;
    logic issue_stage_vld;
    logic issue_stage_rdy;
    logic issue_stage_fill_hsk;
    bti_req_pkt_t issue_stage_pkt;
    beat_ctx_t issue_stage_ctx;
    logic [OST_SLOT_W-1:0] issue_rd_slot;
    logic [OST_SLOT_W-1:0] issue_wr_slot;
    logic issue_rd_found;
    logic issue_wr_found;
    rd_ctx_t issue_rd_ctx;
    wr_ctx_t issue_wr_ctx;
    logic [15:0] issue_trans_id;

    logic rd_out_found;
    logic [OST_SLOT_W-1:0] rd_out_slot;
    rd_ctx_t rd_out_ctx;
    logic wr_out_vld;
    wr_ctx_t wr_out_ctx;
    logic ar_wrap_rsp_vld;
    axi4_ar_pkt_t ar_wrap_rsp_pkt;
    logic aw_wrap_rsp_vld;
    axi4_aw_pkt_t aw_wrap_rsp_pkt;

    logic ar_wrap;
    logic aw_wrap;
    logic ar_alloc_hsk;
    logic aw_alloc_hsk;
    logic bti_req_hsk;
    logic bti_rsp_hsk;
    logic r_hsk;
    logic b_hsk;
    logic ar_wrap_rsp_hsk;
    logic aw_wrap_rsp_hsk;

    function automatic logic [31:0] next_addr(
        input logic [31:0] addr,
        input logic [2:0] size,
        input logic [1:0] burst
    );
        unique case (burst)
        2'd0: next_addr = addr;
        default: next_addr = addr + (32'd1 << size);
        endcase
    endfunction

    function automatic logic [OST_SLOT_W-1:0] slot_idx(
        input int unsigned idx
    );
        slot_idx = idx[OST_SLOT_W-1:0];
    endfunction

    fifo #(
        .DW           (AR_DW),
        .DEPTH        (STG_FIFO_DEPTH),
        .FALL_THROUGH (1'b1)
    ) u_ar_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (axi4_ar_slv.vld),
        .wr_rdy  (axi4_ar_slv.rdy),
        .wr_data (axi4_ar_slv.pkt),
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
        .wr_vld  (axi4_aw_slv.vld),
        .wr_rdy  (axi4_aw_slv.rdy),
        .wr_data (axi4_aw_slv.pkt),
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
        .wr_vld  (axi4_w_slv.vld),
        .wr_rdy  (axi4_w_slv.rdy),
        .wr_data (axi4_w_slv.pkt),
        .rd_vld  (w_fifo_rd_vld),
        .rd_rdy  (w_fifo_rd_rdy),
        .rd_data (w_fifo_rd_data),
        .empty   (),
        .full    ()
    );

    assign ar_pkt = ar_fifo_rd_data;
    assign aw_pkt = aw_fifo_rd_data;
    assign w_pkt = w_fifo_rd_data;
    assign ar_wrap = ar_pkt.burst == AXI4_AR_BURST_WRAP;
    assign aw_wrap = aw_pkt.burst == AXI4_AW_BURST_WRAP;

    ostk #(
        .KEY_W   (8),
        .DW      (1),
        .DEPTH   (OST_DEPTH)
    ) u_rd_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (ar_alloc_hsk),
        .alloc_rdy    (rd_ost_alloc_rdy),
        .alloc_key    (ar_pkt.id),
        .alloc_ctx    (1'b0),
        .alloc_slot   (rd_ost_alloc_slot),
        .lookup_key   ('0),
        .lookup_vld   (),
        .lookup_slot  (),
        .lookup_ctx   (),
        .free_vld     (r_hsk && axi4_r_mst.pkt.last),
        .free_slot    (rd_out_slot),
        .slot_wr_vld  (1'b0),
        .slot_wr_idx  ('0),
        .slot_wr_ctx  ('0),
        .slot_vld     (rd_ost_slot_vld),
        .slot_key_flat(rd_ost_key_flat),
        .slot_ctx_flat(),
        .slot_older_flat(rd_ost_older_flat),
        .empty        (),
        .full         ()
    );

    ostq #(
        .DW    (1),
        .DEPTH (OST_DEPTH)
    ) u_wr_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (aw_alloc_hsk),
        .alloc_rdy    (wr_ost_alloc_rdy),
        .alloc_ctx    (1'b0),
        .alloc_slot   (wr_ost_alloc_slot),
        .head_vld     (wr_ost_head_vld),
        .head_ctx     (),
        .head_slot    (wr_ost_head_slot),
        .free_head    (b_hsk && wr_out_vld),
        .slot_wr_vld  (1'b0),
        .slot_wr_idx  ('0),
        .slot_wr_ctx  ('0),
        .slot_vld     (wr_ost_slot_vld),
        .slot_ctx_flat(),
        .empty        (wr_ost_empty),
        .full         ()
    );

    ostk #(
        .KEY_W   (16),
        .DW      (BEAT_CTX_DW),
        .DEPTH   (OST_DEPTH)
    ) u_beat_ost(
        .clk          (clk),
        .rst_n        (rst_n),
        .alloc_vld    (bti_req_hsk),
        .alloc_rdy    (beat_ost_alloc_rdy),
        .alloc_key    (issue_stage_pkt.trans_id),
        .alloc_ctx    (issue_stage_ctx),
        .alloc_slot   (beat_ost_alloc_slot),
        .lookup_key   (bti_rsp_slv.pkt.trans_id),
        .lookup_vld   (beat_ost_lookup_vld),
        .lookup_slot  (beat_ost_lookup_slot),
        .lookup_ctx   (beat_ost_lookup_ctx),
        .free_vld     (bti_rsp_hsk),
        .free_slot    (beat_ost_lookup_slot),
        .slot_wr_vld  (1'b0),
        .slot_wr_idx  ('0),
        .slot_wr_ctx  ('0),
        .slot_vld     (),
        .slot_key_flat(),
        .slot_ctx_flat(),
        .slot_older_flat(),
        .empty        (),
        .full         ()
    );

    for (genvar i = 0; i < OST_DEPTH; i++) begin : gen_rd_slot_view
        assign rd_ost_key[i] = rd_ost_key_flat[i * 8 +: 8];
        assign issue_rd_candidate_vld[i] = rd_ost_slot_vld[i] &&
            !rd_ctx_mem[i].bti_rsp_pending &&
            !rd_ctx_mem[i].rsp_valid &&
            rd_ctx_mem[i].beat_idx <= rd_ctx_mem[i].axlen;

        for (genvar j = 0; j < OST_DEPTH; j++) begin : gen_older
            assign rd_out_older_same_id[i][j] = rd_ost_slot_vld[j] &&
                rd_ost_key[j] == rd_ost_key[i] &&
                rd_ost_older_flat[j * OST_DEPTH + i];
        end

        assign rd_out_candidate_vld[i] = rd_ost_slot_vld[i] &&
            rd_ctx_mem[i].rsp_valid && !(|rd_out_older_same_id[i]) &&
            !(rd_out_stage_vld && rd_out_stage_slot == slot_idx(i));
    end

    oldest_select #(
        .DEPTH (OST_DEPTH)
    ) u_issue_rd_select(
        .candidate_vld (issue_rd_candidate_vld),
        .older_flat    (rd_ost_older_flat),
        .select_vld    (issue_rd_select_vld),
        .select_slot   (issue_rd_select_slot)
    );

    oldest_select #(
        .DEPTH (OST_DEPTH)
    ) u_rd_out_select(
        .candidate_vld (rd_out_candidate_vld),
        .older_flat    (rd_ost_older_flat),
        .select_vld    (rd_out_select_vld),
        .select_slot   (rd_out_select_slot)
    );

    always_comb begin
        issue_rd_found = issue_rd_select_vld;
        issue_rd_slot = issue_rd_select_slot;
        issue_rd_ctx = issue_rd_select_vld ?
            rd_ctx_mem[issue_rd_select_slot] : '0;
        if (!issue_rd_found && ar_alloc_hsk) begin
            issue_rd_found = 1'b1;
            issue_rd_slot = rd_ost_alloc_slot;
            issue_rd_ctx = rd_ctx_t'{
                axid: ar_pkt.id,
                axaddr: ar_pkt.addr,
                axlen: ar_pkt.len,
                axsize: ar_pkt.size,
                axburst: ar_pkt.burst,
                beat_idx: 8'h00,
                bti_rsp_pending: 1'b0,
                rsp_valid: 1'b0,
                rsp_ok: 1'b0,
                rsp_data: 32'h0
            };
        end
    end

    always_comb begin
        issue_wr_found = 1'b0;
        issue_wr_slot = wr_ost_head_slot;
        issue_wr_ctx = '0;
        if (wr_ost_head_vld) begin
            issue_wr_ctx = wr_ctx_mem[wr_ost_head_slot];
            issue_wr_found = !wr_ctx_mem[wr_ost_head_slot].bti_rsp_pending &&
                !wr_ctx_mem[wr_ost_head_slot].b_valid &&
                w_fifo_rd_vld;
        end else if (wr_ost_empty && aw_alloc_hsk && w_fifo_rd_vld) begin
            issue_wr_found = 1'b1;
            issue_wr_slot = wr_ost_alloc_slot;
            issue_wr_ctx = wr_ctx_t'{
                axid: aw_pkt.id,
                axaddr: aw_pkt.addr,
                axlen: aw_pkt.len,
                axsize: aw_pkt.size,
                axburst: aw_pkt.burst,
                beat_idx: 8'h00,
                bti_rsp_pending: 1'b0,
                pending_last: 1'b0,
                b_valid: 1'b0,
                b_ok: 1'b0
            };
        end
    end

    assign rd_out_stage_rdy = !rd_out_stage_vld;
    assign rd_out_stage_fill_hsk = rd_out_stage_rdy && rd_out_select_vld;
    assign rd_out_found = rd_out_stage_vld;
    assign rd_out_slot = rd_out_stage_slot;
    assign rd_out_ctx = rd_out_stage_ctx;

    assign wr_out_vld = wr_ost_head_vld &&
        wr_ctx_mem[wr_ost_head_slot].b_valid;
    assign wr_out_ctx = wr_ctx_mem[wr_ost_head_slot];

    always_comb begin
        issue_sel_rd = 1'b0;
        issue_sel_wr = 1'b0;
        if (issue_stage_rdy) begin
            if (issue_wr_prio) begin
                if (issue_wr_found)
                    issue_sel_wr = 1'b1;
                else if (issue_rd_found)
                    issue_sel_rd = 1'b1;
            end else begin
                if (issue_rd_found)
                    issue_sel_rd = 1'b1;
                else if (issue_wr_found)
                    issue_sel_wr = 1'b1;
            end
        end
    end

    assign issue_trans_id = next_bti_trans_id == 16'h0000 ?
        16'h0001 : next_bti_trans_id;
    assign issue_stage_rdy = !issue_stage_vld || bti_req_hsk;
    assign issue_stage_fill_hsk = issue_stage_rdy &&
        (issue_sel_rd || issue_sel_wr);
    assign bti_req_hsk = bti_req_mst.vld && bti_req_mst.rdy;
    assign bti_rsp_hsk = bti_rsp_slv.vld && bti_rsp_slv.rdy;
    assign r_hsk = axi4_r_mst.vld && axi4_r_mst.rdy && rd_out_found;
    assign b_hsk = axi4_b_mst.vld && axi4_b_mst.rdy && wr_out_vld;

    assign ar_alloc_hsk = ar_fifo_rd_vld && ar_fifo_rd_rdy && !ar_wrap;
    assign aw_alloc_hsk = aw_fifo_rd_vld && aw_fifo_rd_rdy && !aw_wrap;

    assign ar_fifo_rd_rdy = ar_wrap ? !ar_wrap_rsp_vld :
        rd_ost_alloc_rdy;
    assign aw_fifo_rd_rdy = aw_wrap ? !aw_wrap_rsp_vld :
        wr_ost_alloc_rdy;
    assign w_fifo_rd_rdy = issue_sel_wr && issue_stage_rdy;

    assign bti_req_mst.vld = issue_stage_vld && beat_ost_alloc_rdy;
    assign bti_req_mst.pkt.trans_id = issue_stage_pkt.trans_id;
    assign bti_req_mst.pkt.cmd = issue_stage_pkt.cmd;
    assign bti_req_mst.pkt.addr = issue_stage_pkt.addr;
    assign bti_req_mst.pkt.size = issue_stage_pkt.size;
    assign bti_req_mst.pkt.data = issue_stage_pkt.data;
    assign bti_req_mst.pkt.strobe = issue_stage_pkt.strobe;

    assign bti_rsp_slv.rdy = beat_ost_lookup_vld;

    assign axi4_r_mst.vld = rd_out_found || ar_wrap_rsp_vld;
    assign axi4_r_mst.pkt.id = rd_out_found ? rd_out_ctx.axid :
        ar_wrap_rsp_pkt.id;
    assign axi4_r_mst.pkt.data = rd_out_found ?
        (rd_out_ctx.rsp_ok ? rd_out_ctx.rsp_data : 32'h0) : 32'h0;
    assign axi4_r_mst.pkt.resp = rd_out_found ?
        (rd_out_ctx.rsp_ok ? AXI4_R_RESP_OKAY : AXI4_R_RESP_SLVERR) :
        AXI4_R_RESP_SLVERR;
    assign axi4_r_mst.pkt.last = rd_out_found ?
        (rd_out_ctx.beat_idx == rd_out_ctx.axlen || !rd_out_ctx.rsp_ok) :
        1'b1;

    assign axi4_b_mst.vld = wr_out_vld || aw_wrap_rsp_vld;
    assign axi4_b_mst.pkt.id = wr_out_vld ? wr_out_ctx.axid :
        aw_wrap_rsp_pkt.id;
    assign axi4_b_mst.pkt.resp = wr_out_vld ?
        (wr_out_ctx.b_ok ? AXI4_B_RESP_OKAY : AXI4_B_RESP_SLVERR) :
        AXI4_B_RESP_SLVERR;
    assign ar_wrap_rsp_hsk = ar_wrap_rsp_vld && !rd_out_found &&
        axi4_r_mst.rdy;
    assign aw_wrap_rsp_hsk = aw_wrap_rsp_vld && !wr_out_vld &&
        axi4_b_mst.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            issue_wr_prio <= 1'b0;
            next_bti_trans_id <= 16'h0001;
            issue_stage_vld <= 1'b0;
            issue_stage_pkt <= '0;
            issue_stage_ctx <= '0;
            rd_out_stage_vld <= 1'b0;
            rd_out_stage_slot <= '0;
            rd_out_stage_ctx <= '0;
            ar_wrap_rsp_vld <= 1'b0;
            ar_wrap_rsp_pkt <= '0;
            aw_wrap_rsp_vld <= 1'b0;
            aw_wrap_rsp_pkt <= '0;
        end else begin
            if (ar_wrap_rsp_hsk)
                ar_wrap_rsp_vld <= 1'b0;
            if (!ar_wrap_rsp_vld && ar_fifo_rd_vld && ar_wrap) begin
                ar_wrap_rsp_vld <= 1'b1;
                ar_wrap_rsp_pkt <= ar_pkt;
            end
            if (aw_wrap_rsp_hsk)
                aw_wrap_rsp_vld <= 1'b0;
            if (!aw_wrap_rsp_vld && aw_fifo_rd_vld && aw_wrap) begin
                aw_wrap_rsp_vld <= 1'b1;
                aw_wrap_rsp_pkt <= aw_pkt;
            end
            if (r_hsk)
                rd_out_stage_vld <= 1'b0;
            if (rd_out_stage_fill_hsk) begin
                rd_out_stage_vld <= 1'b1;
                rd_out_stage_slot <= rd_out_select_slot;
                rd_out_stage_ctx <= rd_ctx_mem[rd_out_select_slot];
            end
            if (bti_req_hsk)
                issue_stage_vld <= 1'b0;
            if (issue_stage_fill_hsk) begin
                issue_stage_vld <= 1'b1;
                issue_stage_pkt.trans_id <= issue_trans_id;
                issue_stage_pkt.cmd <= issue_sel_wr ?
                    BTI_REQ_CMD_WRITE : BTI_REQ_CMD_READ;
                issue_stage_pkt.addr <= issue_sel_wr ?
                    issue_wr_ctx.axaddr : issue_rd_ctx.axaddr;
                issue_stage_pkt.size <= issue_sel_wr ?
                    bti_req_size_t'(issue_wr_ctx.axsize[1:0]) :
                    bti_req_size_t'(issue_rd_ctx.axsize[1:0]);
                issue_stage_pkt.data <= issue_sel_wr ? w_pkt.data : 32'h0;
                issue_stage_pkt.strobe <= issue_sel_wr ? w_pkt.strb : 4'h0;
                issue_stage_ctx <= issue_sel_wr ?
                    beat_ctx_t'{kind:BEAT_WRITE, slot:issue_wr_slot,
                        wr_last:w_pkt.last || issue_wr_ctx.beat_idx ==
                        issue_wr_ctx.axlen} :
                    beat_ctx_t'{kind:BEAT_READ, slot:issue_rd_slot,
                        wr_last:1'b0};
                issue_wr_prio <= issue_sel_rd;
                if (issue_trans_id == 16'hffff)
                    next_bti_trans_id <= 16'h0001;
                else
                    next_bti_trans_id <= issue_trans_id + 16'h0001;
            end
        end
    end

    for (genvar i = 0; i < OST_DEPTH; i++) begin : gen_rd_ctx
        wire rd_alloc_hit = ar_alloc_hsk && rd_ost_alloc_slot == slot_idx(i);
        wire rd_issue_hit = issue_stage_fill_hsk && issue_sel_rd &&
            issue_rd_slot == slot_idx(i);
        wire rd_rsp_hit = bti_rsp_hsk &&
            beat_ost_lookup_ctx.kind == BEAT_READ &&
            beat_ost_lookup_ctx.slot == slot_idx(i);
        wire rd_out_hit = r_hsk && rd_out_slot == slot_idx(i);

        always_ff @(posedge clk or negedge rst_n) begin
            if (!rst_n) begin
                rd_ctx_mem[i] <= '0;
            end else begin
                if (rd_alloc_hit) begin
                    rd_ctx_mem[i] <= rd_ctx_t'{
                        axid: ar_pkt.id,
                        axaddr: ar_pkt.addr,
                        axlen: ar_pkt.len,
                        axsize: ar_pkt.size,
                        axburst: ar_pkt.burst,
                        beat_idx: 8'h00,
                        bti_rsp_pending: 1'b0,
                        rsp_valid: 1'b0,
                        rsp_ok: 1'b0,
                        rsp_data: 32'h0
                    };
                end
                if (rd_issue_hit)
                    rd_ctx_mem[i].bti_rsp_pending <= 1'b1;
                if (rd_rsp_hit) begin
                    rd_ctx_mem[i].bti_rsp_pending <= 1'b0;
                    rd_ctx_mem[i].rsp_valid <= 1'b1;
                    rd_ctx_mem[i].rsp_ok <= bti_rsp_slv.pkt.ok;
                    rd_ctx_mem[i].rsp_data <= bti_rsp_slv.pkt.data;
                end
                if (rd_out_hit && !axi4_r_mst.pkt.last) begin
                    rd_ctx_mem[i].rsp_valid <= 1'b0;
                    rd_ctx_mem[i].rsp_ok <= 1'b0;
                    rd_ctx_mem[i].rsp_data <= 32'h0;
                    rd_ctx_mem[i].axaddr <= next_addr(rd_ctx_mem[i].axaddr,
                        rd_ctx_mem[i].axsize, rd_ctx_mem[i].axburst);
                    rd_ctx_mem[i].beat_idx <= rd_ctx_mem[i].beat_idx + 1'b1;
                end
            end
        end
    end

    for (genvar i = 0; i < OST_DEPTH; i++) begin : gen_wr_ctx
        wire wr_alloc_hit = aw_alloc_hsk && wr_ost_alloc_slot == slot_idx(i);
        wire wr_issue_hit = issue_stage_fill_hsk && issue_sel_wr &&
            issue_wr_slot == slot_idx(i);
        wire wr_rsp_hit = bti_rsp_hsk &&
            beat_ost_lookup_ctx.kind == BEAT_WRITE &&
            beat_ost_lookup_ctx.slot == slot_idx(i);

        always_ff @(posedge clk or negedge rst_n) begin
            if (!rst_n) begin
                wr_ctx_mem[i] <= '0;
            end else begin
                if (wr_alloc_hit) begin
                    wr_ctx_mem[i] <= wr_ctx_t'{
                        axid: aw_pkt.id,
                        axaddr: aw_pkt.addr,
                        axlen: aw_pkt.len,
                        axsize: aw_pkt.size,
                        axburst: aw_pkt.burst,
                        beat_idx: 8'h00,
                        bti_rsp_pending: 1'b0,
                        pending_last: 1'b0,
                        b_valid: 1'b0,
                        b_ok: 1'b0
                    };
                end
                if (wr_issue_hit) begin
                    wr_ctx_mem[i].bti_rsp_pending <= 1'b1;
                    wr_ctx_mem[i].pending_last <=
                        w_pkt.last || issue_wr_ctx.beat_idx ==
                        issue_wr_ctx.axlen;
                end
                if (wr_rsp_hit) begin
                    wr_ctx_mem[i].bti_rsp_pending <= 1'b0;
                    wr_ctx_mem[i].pending_last <= 1'b0;
                    if (!bti_rsp_slv.pkt.ok || wr_ctx_mem[i].pending_last) begin
                        wr_ctx_mem[i].b_valid <= 1'b1;
                        wr_ctx_mem[i].b_ok <= bti_rsp_slv.pkt.ok;
                    end else begin
                        wr_ctx_mem[i].axaddr <= next_addr(wr_ctx_mem[i].axaddr,
                            wr_ctx_mem[i].axsize, wr_ctx_mem[i].axburst);
                        wr_ctx_mem[i].beat_idx <= wr_ctx_mem[i].beat_idx + 1'b1;
                    end
                end
                if (b_hsk && wr_ost_head_slot == slot_idx(i))
                    wr_ctx_mem[i].b_valid <= 1'b0;
            end
        end
    end

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && bti_rsp_slv.vld) begin
            assert (beat_ost_lookup_vld)
                else $fatal(1, "axi2bti BTI response has no beat OST entry");
        end
    end
`endif

    wire unused = (|beat_ost_alloc_slot) | (|wr_ost_slot_vld);
endmodule
