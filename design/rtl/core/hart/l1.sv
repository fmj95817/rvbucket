`include "itf/bti_req_if.svh"
`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"
`include "spec/core/cache.svh"
`include "spec/core/hart.svh"

module l1 #(
    parameter bit RO = 1'b0,
    parameter bit FULL_BYPASS = 1'b0,
    parameter int SIZE = `L1D_SIZE,
    parameter int WAY_NUM = `L1D_WAY_NUM,
    parameter int LINE_SIZE = `L1_LINE_SIZE,
    parameter int STG_FIFO_DEPTH = `HART_L1D_STG_FIFO_DEPTH,
    parameter int OST_DEPTH = `HART_L1_OST_DEPTH,
    parameter logic [31:0] BYPASS0_BASE = 32'h0,
    parameter logic [31:0] BYPASS0_SIZE = 32'h0,
    parameter logic [31:0] BYPASS1_BASE = 32'h0,
    parameter logic [31:0] BYPASS1_SIZE = 32'h0,
    parameter logic [31:0] BYPASS2_BASE = 32'h0,
    parameter logic [31:0] BYPASS2_SIZE = 32'h0,
    parameter logic [31:0] BYPASS3_BASE = 32'h0,
    parameter logic [31:0] BYPASS3_SIZE = 32'h0,
    parameter logic [31:0] BYPASS4_BASE = 32'h0,
    parameter logic [31:0] BYPASS4_SIZE = 32'h0,
    parameter logic [31:0] BYPASS5_BASE = 32'h0,
    parameter logic [31:0] BYPASS5_SIZE = 32'h0,
    parameter logic [31:0] BYPASS6_BASE = 32'h0,
    parameter logic [31:0] BYPASS6_SIZE = 32'h0,
    parameter logic [31:0] BYPASS7_BASE = 32'h0,
    parameter logic [31:0] BYPASS7_SIZE = 32'h0
)(
    input logic       clk,
    input logic       rst_n,
    bti_req_if_t.slv  host_bti_req_slv,
    bti_rsp_if_t.mst  host_bti_rsp_mst,
    l1_flush_if_t.slv flush_slv,
    l1_flush_ack_if_t.mst flush_ack_mst,
    axi4_aw_if_t.mst  mem_axi4_aw_mst,
    axi4_w_if_t.mst   mem_axi4_w_mst,
    axi4_b_if_t.slv   mem_axi4_b_slv,
    axi4_ar_if_t.mst  mem_axi4_ar_mst,
    axi4_r_if_t.slv   mem_axi4_r_slv,
    perf_l1_if_t.mst  perf_mst
);
    localparam int BANK_NUM = 4;
    localparam int WORD_NUM = LINE_SIZE / BANK_NUM;
    localparam int WORD_IDX_W = $clog2(WORD_NUM);
    localparam int LINE_OFF_W = $clog2(LINE_SIZE);
    localparam int SET_NUM = SIZE / (WAY_NUM * LINE_SIZE);
    localparam int SET_W = $clog2(SET_NUM);
    localparam int WAY_W = WAY_NUM <= 1 ? 1 : $clog2(WAY_NUM);
    localparam int TAG_W = 32 - LINE_OFF_W - SET_W;
    localparam int DATA_AW = SET_W + WORD_IDX_W;
    localparam int OST_SLOT_W = $clog2(OST_DEPTH);
    localparam logic [WORD_IDX_W-1:0] LAST_BEAT =
        WORD_IDX_W'(WORD_NUM - 1);
    localparam logic [WAY_W-1:0] LAST_WAY = WAY_W'(WAY_NUM - 1);
    localparam logic [SET_W-1:0] LAST_SET = SET_W'(SET_NUM - 1);
    localparam logic [7:0] AXI_LINE_LEN = 8'(WORD_NUM - 1);

    typedef struct packed {
        logic [15:0] trans_id;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        bti_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
    } bti_req_ctx_t;

    typedef struct packed {
        bti_req_ctx_t req;
        logic bypass;
        logic hit;
        logic [WAY_W-1:0] way;
        logic [SET_W-1:0] set;
        logic [TAG_W-1:0] tag;
    } lookup_pipe_t;

    typedef enum logic [1:0] {
        OST_CACHED,
        OST_BYPASS_RD,
        OST_BYPASS_WR
    } ost_kind_t;

    typedef struct packed {
        logic [15:0] trans_id;
        ost_kind_t kind;
        bti_req_cmd_t cmd;
        logic [31:0] addr;
        bti_req_size_t size;
        logic [31:0] req_data;
        logic [3:0] strobe;
        logic split;
        logic [1:0] req_idx;
        logic [1:0] rsp_idx;
        logic rsp_vld;
        logic ok;
        logic [31:0] rsp_data;
    } ost_ctx_t;

    typedef enum logic [4:0] {
        M_IDLE,
        M_LOOKUP,
        M_WB_AW,
        M_WB_READ,
        M_WB_SEND,
        M_WB_B,
        M_REFILL_AR,
        M_REFILL_R,
        M_SERVE_READ,
        M_SERVE_USE,
        M_SERVE_WRITE,
        M_COMPLETE,
        M_CMO_LOOKUP,
        M_CMO_WB_AW,
        M_CMO_WB_READ,
        M_CMO_WB_SEND,
        M_CMO_WB_B,
        M_CMO_L2_AR,
        M_CMO_L2_R,
        M_FLUSH_CHECK,
        M_FLUSH_WB_AW,
        M_FLUSH_WB_READ,
        M_FLUSH_WB_SEND,
        M_FLUSH_WB_B,
        M_FLUSH_ACK
    } miss_state_t;

    typedef struct packed {
        logic [OST_SLOT_W-1:0] slot;
        logic [31:0] addr;
        bti_req_cmd_t cmd;
        bti_req_size_t size;
        logic [31:0] data;
        logic [3:0] strobe;
        logic [WAY_W-1:0] way;
        logic ok;
    } hit_pipe_t;

    localparam int STG_DW = $bits(bti_req_ctx_t);
    localparam int LOOKUP_DW = $bits(lookup_pipe_t);
    localparam int OST_CTX_DW = $bits(ost_ctx_t);

    logic [TAG_W-1:0] tag_ram[WAY_NUM][SET_NUM];
    logic valid_ram[WAY_NUM][SET_NUM];
    logic dirty_ram[WAY_NUM][SET_NUM];
    logic [WAY_W-1:0] replace_way[SET_NUM];

    sram_rw_if_t #(DATA_AW, 8) data_r_if[WAY_NUM * BANK_NUM]();
    sram_rw_if_t #(DATA_AW, 8) data_w_if[WAY_NUM * BANK_NUM]();
    logic data_r_cs[WAY_NUM][BANK_NUM];
    logic [DATA_AW-1:0] data_r_addr[WAY_NUM][BANK_NUM];
    logic [7:0] data_bank_rdata[WAY_NUM][BANK_NUM];
    logic data_w_cs[WAY_NUM][BANK_NUM];
    logic [DATA_AW-1:0] data_w_addr[WAY_NUM][BANK_NUM];
    logic [7:0] data_wdata[WAY_NUM][BANK_NUM];

    logic stg_wr_rdy;
    logic [STG_DW-1:0] stg_wr_data;
    logic fifo_rd_vld;
    logic fifo_rd_rdy;
    logic [STG_DW-1:0] fifo_rd_data;
    logic req_pipe_vld;
    logic req_pipe_rdy;
    logic [STG_DW-1:0] req_pipe_data;
    logic stg_rd_vld;
    logic stg_rd_rdy;
    logic [LOOKUP_DW-1:0] stg_rd_data;
    logic stg_full;
    bti_req_ctx_t stg_wr_req;
    bti_req_ctx_t req_pipe_req;
    bti_req_ctx_t stg_req;
    lookup_pipe_t lookup_wr;
    lookup_pipe_t lookup_rd;
    logic [LOOKUP_DW-1:0] lookup_wr_data;

    logic ost_alloc_vld;
    logic ost_alloc_rdy;
    logic [OST_CTX_DW-1:0] ost_alloc_ctx_raw;
    logic [OST_SLOT_W-1:0] ost_alloc_slot;
    logic ost_head_vld;
    logic [OST_CTX_DW-1:0] ost_head_ctx_raw;
    logic [OST_SLOT_W-1:0] ost_head_slot;
    logic ost_free_head;
    logic ost_slot_wr_vld;
    logic [OST_SLOT_W-1:0] ost_slot_wr_idx;
    logic [OST_CTX_DW-1:0] ost_slot_wr_ctx_raw;
    logic [OST_DEPTH-1:0] ost_slot_vld;
    logic [OST_DEPTH*OST_CTX_DW-1:0] ost_slot_ctx_flat;
    logic ost_empty;
    logic ost_full;
    ost_ctx_t ost_alloc_ctx;
    ost_ctx_t ost_head_ctx;
    ost_ctx_t ost_slot_ctx[OST_DEPTH];
    ost_ctx_t ost_slot_wr_ctx;

    miss_state_t miss_state;
    bti_req_ctx_t active_req;
    logic [OST_SLOT_W-1:0] active_slot;
    logic [2:0] active_byte_idx;
    logic [31:0] active_rsp_data;
    logic active_ok;
    logic active_precounted_hit;
    logic [SET_W-1:0] active_set;
    logic [TAG_W-1:0] active_tag;
    logic [WAY_W-1:0] active_way;
    logic active_way_vld;
    logic [31:0] active_line_addr;
    logic [31:0] wb_line_addr;
    logic [WORD_IDX_W-1:0] beat_idx;
    logic refill_ok;
    logic [SET_W-1:0] flush_set;
    logic [WAY_W-1:0] flush_way;
    logic flush_pending;

    logic hit_pipe_vld;
    hit_pipe_t hit_pipe;

    logic wr_issue_vld;
    logic [OST_SLOT_W-1:0] wr_issue_slot;
    ost_ctx_t wr_issue_ctx;
    logic wr_issue_aw_done;
    logic wr_issue_w_done;

    logic lookup_hit;
    logic [WAY_W-1:0] lookup_way;
    logic lookup_active_conflict;
    logic front_active_conflict;
    logic active_hit;
    logic [WAY_W-1:0] active_hit_way;
    logic active_found_invalid;
    logic [WAY_W-1:0] active_victim_way;
    logic active_victim_dirty;
    logic [TAG_W-1:0] active_victim_tag;

    logic has_bypass_ost;
    logic rd_issue_found;
    logic [OST_SLOT_W-1:0] rd_issue_slot;
    ost_ctx_t rd_issue_ctx;
    logic rd_issue_stage_vld;
    logic rd_issue_stage_rdy;
    logic rd_issue_stage_hsk;
    logic [OST_SLOT_W-1:0] rd_issue_stage_slot;
    ost_ctx_t rd_issue_stage_ctx;
    logic wr_load_found;
    logic [OST_SLOT_W-1:0] wr_load_slot;
    ost_ctx_t wr_load_ctx;

    logic accept_fast;
    logic accept_ro_error;
    logic accept_active;
    logic accept_bypass_rd;
    logic accept_bypass_wr;
    logic accept_req;
    logic flush_active;
    logic flush_start;
    logic cmo_active;
    logic fast_port_blocked;
    logic hit_pipe_retire;
    logic hit_pipe_write_blocked;

    logic read_launch;
    logic read_all_ways;
    logic read_word_mode;
    logic [WAY_W-1:0] read_way;
    logic [SET_W-1:0] read_set;
    logic [WORD_IDX_W-1:0] read_word_idx;
    logic [31:0] read_addr;
    logic [31:0] hit_pipe_read_data;
    logic [31:0] active_read_data;
    logic [31:0] wb_read_data;

    logic write_launch;
    logic write_word_mode;
    logic [WAY_W-1:0] write_way;
    logic [SET_W-1:0] write_set;
    logic [WORD_IDX_W-1:0] write_word_idx;
    logic [31:0] write_addr;
    logic [31:0] write_data;
    logic [3:0] write_strobe;
    logic [2:0] write_byte_base;
    logic [2:0] write_byte_count;

    logic miss_complete_req;
    logic bypass_r_update;
    logic bypass_b_update;
    logic wr_issue_update;
    logic hit_pipe_update;
    logic rd_issue_update;
    logic slot_writer_busy;
    logic slot_pre_busy;
    logic bypass_r_candidate;
    logic bypass_b_candidate;
    logic [OST_SLOT_W-1:0] bypass_r_slot;
    logic [OST_SLOT_W-1:0] bypass_b_slot;
    ost_ctx_t bypass_r_ctx;
    ost_ctx_t bypass_b_ctx;
    logic ar_sel_rd_issue;
    logic ar_sel_front_bypass;
    logic [DATA_AW-1:0] read_base_addr;
    logic [DATA_AW-1:0] write_base_addr;

    wire host_req_hsk = host_bti_req_slv.vld && host_bti_req_slv.rdy;
    wire host_rsp_hsk = host_bti_rsp_mst.vld && host_bti_rsp_mst.rdy;
    wire aw_hsk = mem_axi4_aw_mst.vld && mem_axi4_aw_mst.rdy;
    wire w_hsk = mem_axi4_w_mst.vld && mem_axi4_w_mst.rdy;
    wire b_hsk = mem_axi4_b_slv.vld && mem_axi4_b_slv.rdy;
    wire ar_hsk = mem_axi4_ar_mst.vld && mem_axi4_ar_mst.rdy;
    wire r_hsk = mem_axi4_r_slv.vld && mem_axi4_r_slv.rdy;

    wire [2:0] lookup_byte_num = req_byte_num(req_pipe_req.size);
    wire [31:0] lookup_end_addr = req_pipe_req.addr +
        {29'b0, lookup_byte_num} - 32'd1;
    wire lookup_cross_line = line_addr(req_pipe_req.addr) !=
        line_addr(lookup_end_addr);
    wire [SET_W-1:0] lookup_set = req_set(req_pipe_req.addr);
    wire [TAG_W-1:0] lookup_tag = req_tag(req_pipe_req.addr);
    wire [SET_W-1:0] front_set0 = lookup_rd.set;

    wire [31:0] active_cur_addr = active_req.addr +
        {29'b0, active_byte_idx};
    wire [2:0] active_total_bytes = req_byte_num(active_req.size);
    wire [SET_W-1:0] active_cur_set = req_set(active_cur_addr);
    wire [TAG_W-1:0] active_cur_tag = req_tag(active_cur_addr);
    wire [31:0] active_cur_line = line_addr(active_cur_addr);
    wire [6:0] active_line_left = 7'(LINE_SIZE) -
        {1'b0, active_cur_addr[LINE_OFF_W-1:0]};
    wire [2:0] active_bytes_left = active_total_bytes - active_byte_idx;
    wire [2:0] active_fragment_bytes =
        active_line_left < {4'b0, active_bytes_left} ?
        active_line_left[2:0] :
        active_bytes_left;
    wire active_fragment_last =
        active_byte_idx + active_fragment_bytes >= active_total_bytes;

    wire front_is_bypass = lookup_rd.bypass;
    wire front_is_write = stg_req.cmd == BTI_REQ_CMD_WRITE;
    wire front_is_read = stg_req.cmd == BTI_REQ_CMD_READ;
    wire front_is_cmo = req_is_cmo(stg_req.cmd);
    wire front_is_normal = front_is_read || front_is_write;
    wire front_hit_current = lookup_rd.hit &&
        valid_ram[lookup_rd.way][lookup_rd.set] &&
        tag_ram[lookup_rd.way][lookup_rd.set] == lookup_rd.tag;
    wire front_fast_hit = front_hit_current && !front_active_conflict;
    wire wr_issue_complete = wr_issue_vld &&
        (wr_issue_aw_done || aw_hsk) && (wr_issue_w_done || w_hsk);

    function automatic logic [2:0] req_byte_num(
        input bti_req_size_t size
    );
        unique case (size)
        BTI_REQ_SIZE_B1: req_byte_num = 3'd1;
        BTI_REQ_SIZE_B2: req_byte_num = 3'd2;
        default: req_byte_num = 3'd4;
        endcase
    endfunction

    function automatic logic req_is_cmo(input bti_req_cmd_t cmd);
        req_is_cmo = cmd == BTI_REQ_CMD_CBO_INVAL ||
            cmd == BTI_REQ_CMD_CBO_CLEAN ||
            cmd == BTI_REQ_CMD_CBO_FLUSH;
    endfunction

    function automatic logic cmo_cleans(input bti_req_cmd_t cmd);
        cmo_cleans = cmd == BTI_REQ_CMD_CBO_CLEAN ||
            cmd == BTI_REQ_CMD_CBO_FLUSH;
    endfunction

    function automatic logic cmo_invalidates(input bti_req_cmd_t cmd);
        cmo_invalidates = cmd == BTI_REQ_CMD_CBO_INVAL ||
            cmd == BTI_REQ_CMD_CBO_FLUSH;
    endfunction

    function automatic logic [31:0] cmo_axi_user(input bti_req_cmd_t cmd);
        unique case (cmd)
        BTI_REQ_CMD_CBO_INVAL:
            cmo_axi_user = `AXI4_USER_MAKE_CMO(`AXI4_USER_CMO_OP_INVAL);
        BTI_REQ_CMD_CBO_CLEAN:
            cmo_axi_user = `AXI4_USER_MAKE_CMO(`AXI4_USER_CMO_OP_CLEAN);
        default:
            cmo_axi_user = `AXI4_USER_MAKE_CMO(`AXI4_USER_CMO_OP_FLUSH);
        endcase
    endfunction

    function automatic logic [31:0] line_addr(input logic [31:0] addr);
        line_addr = {addr[31:LINE_OFF_W], {LINE_OFF_W{1'b0}}};
    endfunction

    function automatic logic [SET_W-1:0] req_set(input logic [31:0] addr);
        req_set = addr[LINE_OFF_W +: SET_W];
    endfunction

    function automatic logic [TAG_W-1:0] req_tag(input logic [31:0] addr);
        req_tag = addr[31 -: TAG_W];
    endfunction

    function automatic logic [WAY_W-1:0] way_idx(input int unsigned idx);
        way_idx = WAY_W'(idx);
    endfunction

    function automatic logic addr_in_range(
        input logic [31:0] addr,
        input logic [31:0] base,
        input logic [31:0] range_size
    );
        addr_in_range = range_size != 0 && addr >= base &&
            addr < base + range_size;
    endfunction

    function automatic logic bypass_addr(input logic [31:0] addr);
        bypass_addr = FULL_BYPASS ||
            addr_in_range(addr, BYPASS0_BASE, BYPASS0_SIZE) ||
            addr_in_range(addr, BYPASS1_BASE, BYPASS1_SIZE) ||
            addr_in_range(addr, BYPASS2_BASE, BYPASS2_SIZE) ||
            addr_in_range(addr, BYPASS3_BASE, BYPASS3_SIZE) ||
            addr_in_range(addr, BYPASS4_BASE, BYPASS4_SIZE) ||
            addr_in_range(addr, BYPASS5_BASE, BYPASS5_SIZE) ||
            addr_in_range(addr, BYPASS6_BASE, BYPASS6_SIZE) ||
            addr_in_range(addr, BYPASS7_BASE, BYPASS7_SIZE);
    endfunction

    function automatic logic req_cross_word(
        input logic [31:0] addr,
        input bti_req_size_t size
    );
        req_cross_word = {1'b0, addr[1:0]} + req_byte_num(size) > 3'd4;
    endfunction

    function automatic logic [1:0] bypass_beat_num(input ost_ctx_t ctx);
        bypass_beat_num = ctx.split ? 2'd2 : 2'd1;
    endfunction

    function automatic logic [31:0] bypass_beat_addr(input ost_ctx_t ctx);
        bypass_beat_addr = {ctx.addr[31:2], 2'b00} +
            ({30'b0, ctx.req_idx} << 2);
    endfunction

    function automatic logic [31:0] bypass_write_data(input ost_ctx_t ctx);
        if (ctx.req_idx == 0)
            bypass_write_data = ctx.req_data << ({3'b0, ctx.addr[1:0]} << 3);
        else
            bypass_write_data = ctx.req_data >>
                ((3'd4 - {1'b0, ctx.addr[1:0]}) << 3);
    endfunction

    function automatic logic [3:0] bypass_write_strobe(input ost_ctx_t ctx);
        if (ctx.req_idx == 0)
            bypass_write_strobe = (ctx.strobe << ctx.addr[1:0]) & 4'hf;
        else
            bypass_write_strobe = ctx.strobe >>
                (3'd4 - {1'b0, ctx.addr[1:0]});
    endfunction

    function automatic logic axi_resp_ok(input logic [1:0] resp);
        axi_resp_ok = resp == 2'b00;
    endfunction

    function automatic logic [31:0] byte_mask(input logic [2:0] count);
        unique case (count)
        3'd1: byte_mask = 32'h000000ff;
        3'd2: byte_mask = 32'h0000ffff;
        3'd3: byte_mask = 32'h00ffffff;
        default: byte_mask = 32'hffffffff;
        endcase
    endfunction

`ifndef SYNTHESIS
    initial begin
        assert (WAY_NUM > 0);
        assert (SET_NUM > 1);
        assert ((SET_NUM & (SET_NUM - 1)) == 0);
        assert ((LINE_SIZE & (LINE_SIZE - 1)) == 0);
        assert ((OST_DEPTH & (OST_DEPTH - 1)) == 0);
        assert (OST_DEPTH <= 256);
    end
`endif

    always_comb begin
        stg_wr_req.trans_id = host_bti_req_slv.pkt.trans_id;
        stg_wr_req.cmd = host_bti_req_slv.pkt.cmd;
        stg_wr_req.addr = host_bti_req_slv.pkt.addr;
        stg_wr_req.size = host_bti_req_slv.pkt.size;
        stg_wr_req.data = host_bti_req_slv.pkt.data;
        stg_wr_req.strobe = host_bti_req_slv.pkt.strobe;
    end
    assign stg_wr_data = stg_wr_req;
    assign req_pipe_req = bti_req_ctx_t'(req_pipe_data);
    assign lookup_wr_data = lookup_wr;
    assign lookup_rd = lookup_pipe_t'(stg_rd_data);
    assign stg_req = lookup_rd.req;

    fifo #(
        .DW    (STG_DW),
        .DEPTH (STG_FIFO_DEPTH)
    ) u_req_fifo(
        .clk     (clk),
        .rst_n   (rst_n),
        .clear   (1'b0),
        .wr_vld  (host_bti_req_slv.vld),
        .wr_rdy  (stg_wr_rdy),
        .wr_data (stg_wr_data),
        .rd_vld  (fifo_rd_vld),
        .rd_rdy  (fifo_rd_rdy),
        .rd_data (fifo_rd_data),
        .empty   (),
        .full    (stg_full)
    );

    vld_reg_slice #(
        .DW (STG_DW)
    ) u_req_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (fifo_rd_vld),
        .src_rdy  (fifo_rd_rdy),
        .src_data (fifo_rd_data),
        .dst_vld  (req_pipe_vld),
        .dst_rdy  (req_pipe_rdy),
        .dst_data (req_pipe_data)
    );

    vld_reg_slice #(
        .DW (LOOKUP_DW)
    ) u_lookup_slice(
        .clk      (clk),
        .rst_n    (rst_n),
        .clear    (1'b0),
        .src_vld  (req_pipe_vld),
        .src_rdy  (req_pipe_rdy),
        .src_data (lookup_wr_data),
        .dst_vld  (stg_rd_vld),
        .dst_rdy  (stg_rd_rdy),
        .dst_data (stg_rd_data)
    );

    assign ost_alloc_ctx_raw = ost_alloc_ctx;
    assign ost_head_ctx = ost_ctx_t'(ost_head_ctx_raw);
    assign ost_slot_wr_ctx_raw = ost_slot_wr_ctx;

    ostq #(
        .DW    (OST_CTX_DW),
        .DEPTH (OST_DEPTH)
    ) u_ost(
        .clk           (clk),
        .rst_n         (rst_n),
        .alloc_vld     (ost_alloc_vld),
        .alloc_rdy     (ost_alloc_rdy),
        .alloc_ctx     (ost_alloc_ctx_raw),
        .alloc_slot    (ost_alloc_slot),
        .head_vld      (ost_head_vld),
        .head_ctx      (ost_head_ctx_raw),
        .head_slot     (ost_head_slot),
        .free_head     (ost_free_head),
        .slot_wr_vld   (ost_slot_wr_vld),
        .slot_wr_idx   (ost_slot_wr_idx),
        .slot_wr_ctx   (ost_slot_wr_ctx_raw),
        .slot_vld      (ost_slot_vld),
        .slot_ctx_flat (ost_slot_ctx_flat),
        .empty         (ost_empty),
        .full          (ost_full)
    );

    for (genvar i = 0; i < OST_DEPTH; i++) begin : gen_ost_view
        assign ost_slot_ctx[i] = ost_ctx_t'(
            ost_slot_ctx_flat[i * OST_CTX_DW +: OST_CTX_DW]);
    end

    for (genvar w = 0; w < WAY_NUM; w++) begin : gen_data_way
        for (genvar b = 0; b < BANK_NUM; b++) begin : gen_data_bank
            localparam int IDX = w * BANK_NUM + b;

            assign data_r_if[IDX].cs = data_r_cs[w][b];
            assign data_r_if[IDX].addr = data_r_addr[w][b];
            assign data_r_if[IDX].wen = 1'b0;
            assign data_r_if[IDX].wdata = 8'b0;
            assign data_bank_rdata[w][b] = data_r_if[IDX].rdata;

            assign data_w_if[IDX].cs = data_w_cs[w][b];
            assign data_w_if[IDX].addr = data_w_addr[w][b];
            assign data_w_if[IDX].wen = data_w_cs[w][b];
            assign data_w_if[IDX].wdata = data_wdata[w][b];

            dp_sram #(
                .AW (DATA_AW),
                .DW (8)
            ) u_data_sram(
                .clk         (clk),
                .sram_r_slv  (data_r_if[IDX]),
                .sram_w_slv  (data_w_if[IDX])
            );
        end
    end

    always_comb begin
        lookup_hit = 1'b0;
        lookup_way = '0;
        for (int unsigned i = 0; i < WAY_NUM; i++) begin
            if (!lookup_hit && valid_ram[i][lookup_set] &&
                tag_ram[i][lookup_set] == lookup_tag) begin
                lookup_hit = 1'b1;
                lookup_way = way_idx(i);
            end
        end

        lookup_active_conflict = 1'b0;
        if (miss_state != M_IDLE && active_way_vld) begin
            lookup_active_conflict = lookup_hit &&
                lookup_set == active_set && lookup_way == active_way;
        end else if (miss_state == M_LOOKUP) begin
            lookup_active_conflict = lookup_set == active_cur_set;
        end

        lookup_wr.req = req_pipe_req;
        lookup_wr.bypass = bypass_addr(req_pipe_req.addr);
        lookup_wr.hit = (req_pipe_req.cmd == BTI_REQ_CMD_READ ||
            req_pipe_req.cmd == BTI_REQ_CMD_WRITE) && !lookup_wr.bypass &&
            !(RO && req_pipe_req.cmd == BTI_REQ_CMD_WRITE) &&
            !lookup_cross_line && lookup_hit && !lookup_active_conflict;
        lookup_wr.way = lookup_way;
        lookup_wr.set = lookup_set;
        lookup_wr.tag = lookup_tag;
    end

    always_comb begin
        front_active_conflict = 1'b0;
        if (miss_state != M_IDLE && active_way_vld) begin
            front_active_conflict = lookup_rd.hit &&
                lookup_rd.set == active_set &&
                lookup_rd.way == active_way;
        end else if (miss_state == M_LOOKUP) begin
            front_active_conflict = lookup_rd.set == active_cur_set;
        end
    end

    always_comb begin
        active_hit = 1'b0;
        active_hit_way = '0;
        active_found_invalid = 1'b0;
        active_victim_way = replace_way[active_cur_set];
        for (int unsigned i = 0; i < WAY_NUM; i++) begin
            if (!active_hit && valid_ram[i][active_cur_set] &&
                tag_ram[i][active_cur_set] == active_cur_tag) begin
                active_hit = 1'b1;
                active_hit_way = way_idx(i);
            end
            if (!active_found_invalid && !valid_ram[i][active_cur_set]) begin
                active_found_invalid = 1'b1;
                active_victim_way = way_idx(i);
            end
        end
        active_victim_dirty =
            valid_ram[active_victim_way][active_cur_set] &&
            dirty_ram[active_victim_way][active_cur_set];
        active_victim_tag = tag_ram[active_victim_way][active_cur_set];
    end

    always_comb begin
        has_bypass_ost = 1'b0;
        rd_issue_found = 1'b0;
        rd_issue_slot = '0;
        rd_issue_ctx = '0;
        wr_load_found = 1'b0;
        wr_load_slot = '0;
        wr_load_ctx = '0;
        for (int unsigned i = 0; i < OST_DEPTH; i++) begin
            logic [OST_SLOT_W-1:0] idx;
            idx = ost_head_slot + OST_SLOT_W'(i);
            if (ost_slot_vld[idx] &&
                (ost_slot_ctx[idx].kind == OST_BYPASS_RD ||
                    ost_slot_ctx[idx].kind == OST_BYPASS_WR))
                has_bypass_ost = 1'b1;
            if (!rd_issue_found && ost_slot_vld[idx] &&
                ost_slot_ctx[idx].kind == OST_BYPASS_RD &&
                !(rd_issue_stage_vld && idx == rd_issue_stage_slot) &&
                !ost_slot_ctx[idx].rsp_vld &&
                ost_slot_ctx[idx].req_idx == ost_slot_ctx[idx].rsp_idx &&
                ost_slot_ctx[idx].req_idx <
                    bypass_beat_num(ost_slot_ctx[idx])) begin
                rd_issue_found = 1'b1;
                rd_issue_slot = idx;
                rd_issue_ctx = ost_slot_ctx[idx];
            end
            if (!wr_load_found && ost_slot_vld[idx] &&
                ost_slot_ctx[idx].kind == OST_BYPASS_WR &&
                !ost_slot_ctx[idx].rsp_vld &&
                ost_slot_ctx[idx].req_idx == ost_slot_ctx[idx].rsp_idx &&
                ost_slot_ctx[idx].req_idx <
                    bypass_beat_num(ost_slot_ctx[idx])) begin
                wr_load_found = 1'b1;
                wr_load_slot = idx;
                wr_load_ctx = ost_slot_ctx[idx];
            end
        end
    end

    assign flush_active = miss_state >= M_FLUSH_CHECK;
    assign cmo_active = miss_state >= M_CMO_LOOKUP &&
        miss_state <= M_CMO_L2_R;
    assign flush_start = miss_state == M_IDLE && flush_pending && ost_empty &&
        !hit_pipe_vld && !wr_issue_vld;
    assign fast_port_blocked = flush_active || cmo_active ||
        miss_state == M_WB_READ || miss_state == M_WB_SEND ||
        miss_state == M_SERVE_READ || miss_state == M_SERVE_USE ||
        miss_state == M_SERVE_WRITE ||
        miss_state == M_FLUSH_WB_READ || miss_state == M_FLUSH_WB_SEND;

    assign miss_complete_req = miss_state == M_COMPLETE;

    always_comb begin
        bypass_r_slot = mem_axi4_r_slv.pkt.id[OST_SLOT_W-1:0];
        bypass_b_slot = mem_axi4_b_slv.pkt.id[OST_SLOT_W-1:0];
        bypass_r_ctx = ost_slot_ctx[bypass_r_slot];
        bypass_b_ctx = ost_slot_ctx[bypass_b_slot];
        bypass_r_candidate = miss_state != M_REFILL_R &&
            miss_state != M_CMO_L2_R &&
            mem_axi4_r_slv.vld && ost_slot_vld[bypass_r_slot] &&
            bypass_r_ctx.kind == OST_BYPASS_RD &&
            bypass_r_ctx.rsp_idx < bypass_r_ctx.req_idx;
        bypass_b_candidate = miss_state != M_WB_B &&
            miss_state != M_CMO_WB_B &&
            miss_state != M_FLUSH_WB_B && mem_axi4_b_slv.vld &&
            ost_slot_vld[bypass_b_slot] &&
            bypass_b_ctx.kind == OST_BYPASS_WR &&
            bypass_b_ctx.rsp_idx < bypass_b_ctx.req_idx;
    end

    assign slot_pre_busy = miss_complete_req || bypass_r_candidate ||
        bypass_b_candidate || wr_issue_complete || hit_pipe_vld ||
        rd_issue_stage_vld;
    assign hit_pipe_write_blocked = hit_pipe_vld &&
        hit_pipe.cmd == BTI_REQ_CMD_WRITE &&
        ((miss_state == M_REFILL_R && r_hsk) ||
            miss_state == M_SERVE_WRITE);
    assign ar_sel_rd_issue = miss_state == M_IDLE && rd_issue_found &&
        !slot_pre_busy && rd_issue_stage_rdy;
    assign ar_sel_front_bypass = miss_state == M_IDLE && stg_rd_vld &&
        front_is_bypass && front_is_read && ost_alloc_rdy &&
        !rd_issue_found && !slot_pre_busy && !flush_pending;

    always_comb begin
        accept_fast = 1'b0;
        accept_ro_error = 1'b0;
        accept_active = 1'b0;
        accept_bypass_rd = 1'b0;
        accept_bypass_wr = 1'b0;

        if (stg_rd_vld && !flush_pending && !flush_active &&
            ost_alloc_rdy) begin
            if (front_is_cmo) begin
                if (miss_state == M_IDLE && ost_empty && !hit_pipe_vld &&
                    !wr_issue_vld && !has_bypass_ost)
                    accept_active = 1'b1;
            end else if (!front_is_normal) begin
                accept_ro_error = 1'b1;
            end else if (front_is_bypass) begin
                if (RO && front_is_write) begin
                    accept_ro_error = 1'b1;
                end else if (front_is_read) begin
                    accept_bypass_rd = ar_sel_front_bypass && ar_hsk;
                end else if (miss_state == M_IDLE && !wr_issue_vld &&
                    !wr_load_found) begin
                    accept_bypass_wr = 1'b1;
                end
            end else if (RO && front_is_write) begin
                accept_ro_error = 1'b1;
            end else if (front_fast_hit && !fast_port_blocked &&
                (!hit_pipe_vld || hit_pipe_retire) &&
                !(front_is_write && miss_state == M_REFILL_R && r_hsk)) begin
                accept_fast = 1'b1;
            end else if (miss_state == M_IDLE && !has_bypass_ost &&
                !front_fast_hit) begin
                accept_active = 1'b1;
            end
        end
    end

    assign accept_req = accept_fast || accept_ro_error || accept_active ||
        accept_bypass_rd || accept_bypass_wr;
    assign stg_rd_rdy = accept_req;
    assign ost_alloc_vld = accept_req;
    assign rd_issue_stage_rdy = !rd_issue_stage_vld;

    always_comb begin
        ost_alloc_ctx = '0;
        ost_alloc_ctx.trans_id = stg_req.trans_id;
        ost_alloc_ctx.kind = accept_bypass_rd ? OST_BYPASS_RD :
            (accept_bypass_wr ? OST_BYPASS_WR : OST_CACHED);
        ost_alloc_ctx.cmd = stg_req.cmd;
        ost_alloc_ctx.addr = stg_req.addr;
        ost_alloc_ctx.size = stg_req.size;
        ost_alloc_ctx.req_data = stg_req.data;
        ost_alloc_ctx.strobe = stg_req.strobe;
        ost_alloc_ctx.split = req_cross_word(stg_req.addr, stg_req.size);
        ost_alloc_ctx.req_idx = accept_bypass_rd ? 2'd1 : 2'd0;
        ost_alloc_ctx.rsp_idx = 2'd0;
        ost_alloc_ctx.rsp_vld = accept_ro_error;
        ost_alloc_ctx.ok = 1'b0;
        ost_alloc_ctx.rsp_data = 32'b0;
    end

    always_comb begin
        bypass_r_update = 1'b0;
        bypass_b_update = 1'b0;
        wr_issue_update = 1'b0;
        hit_pipe_update = 1'b0;
        rd_issue_update = 1'b0;
        slot_writer_busy = 1'b0;

        ost_slot_wr_vld = 1'b0;
        ost_slot_wr_idx = '0;
        ost_slot_wr_ctx = '0;

        mem_axi4_r_slv.rdy = 1'b0;
        mem_axi4_b_slv.rdy = 1'b0;

        if (miss_state == M_REFILL_R || miss_state == M_CMO_L2_R) begin
            mem_axi4_r_slv.rdy = 1'b1;
        end else if (miss_state == M_WB_B ||
            miss_state == M_CMO_WB_B ||
            miss_state == M_FLUSH_WB_B) begin
            mem_axi4_b_slv.rdy = 1'b1;
        end else if (miss_complete_req) begin
            ost_slot_wr_vld = 1'b1;
            ost_slot_wr_idx = active_slot;
            ost_slot_wr_ctx = ost_slot_ctx[active_slot];
            ost_slot_wr_ctx.rsp_vld = 1'b1;
            ost_slot_wr_ctx.ok = active_ok;
            ost_slot_wr_ctx.rsp_data =
                active_req.cmd == BTI_REQ_CMD_READ ? active_rsp_data : 32'b0;
            slot_writer_busy = 1'b1;
        end else if (bypass_r_candidate) begin
            mem_axi4_r_slv.rdy = 1'b1;
            bypass_r_update = 1'b1;
            ost_slot_wr_vld = 1'b1;
            ost_slot_wr_idx = bypass_r_slot;
            ost_slot_wr_ctx = bypass_r_ctx;
            if (bypass_r_ctx.rsp_idx == 0) begin
                    ost_slot_wr_ctx.rsp_data = mem_axi4_r_slv.pkt.data >>
                    ({3'b0, bypass_r_ctx.addr[1:0]} << 3);
                    ost_slot_wr_ctx.ok =
                        axi_resp_ok(mem_axi4_r_slv.pkt.resp);
            end else begin
                ost_slot_wr_ctx.rsp_data = bypass_r_ctx.rsp_data |
                    (mem_axi4_r_slv.pkt.data <<
                        ((3'd4 - {1'b0, bypass_r_ctx.addr[1:0]}) << 3));
                ost_slot_wr_ctx.ok = bypass_r_ctx.ok &&
                    axi_resp_ok(mem_axi4_r_slv.pkt.resp);
            end
            ost_slot_wr_ctx.rsp_idx = bypass_r_ctx.rsp_idx + 1'b1;
            ost_slot_wr_ctx.rsp_vld = bypass_r_ctx.rsp_idx + 1'b1 ==
                bypass_beat_num(bypass_r_ctx);
            slot_writer_busy = 1'b1;
        end else if (bypass_b_candidate) begin
            mem_axi4_b_slv.rdy = 1'b1;
            bypass_b_update = 1'b1;
            ost_slot_wr_vld = 1'b1;
            ost_slot_wr_idx = bypass_b_slot;
            ost_slot_wr_ctx = bypass_b_ctx;
            ost_slot_wr_ctx.ok = bypass_b_ctx.rsp_idx == 0 ?
                axi_resp_ok(mem_axi4_b_slv.pkt.resp) :
                bypass_b_ctx.ok && axi_resp_ok(mem_axi4_b_slv.pkt.resp);
            ost_slot_wr_ctx.rsp_data = 32'b0;
            ost_slot_wr_ctx.rsp_idx = bypass_b_ctx.rsp_idx + 1'b1;
            ost_slot_wr_ctx.rsp_vld = bypass_b_ctx.rsp_idx + 1'b1 ==
                bypass_beat_num(bypass_b_ctx);
            slot_writer_busy = 1'b1;
        end

        if (!slot_writer_busy && wr_issue_complete) begin
            wr_issue_update = 1'b1;
            ost_slot_wr_vld = 1'b1;
            ost_slot_wr_idx = wr_issue_slot;
            ost_slot_wr_ctx = ost_slot_ctx[wr_issue_slot];
            ost_slot_wr_ctx.req_idx =
                ost_slot_ctx[wr_issue_slot].req_idx + 1'b1;
            slot_writer_busy = 1'b1;
        end

        if (!slot_writer_busy && hit_pipe_vld && !hit_pipe_write_blocked) begin
            hit_pipe_update = 1'b1;
            ost_slot_wr_vld = 1'b1;
            ost_slot_wr_idx = hit_pipe.slot;
            ost_slot_wr_ctx = ost_slot_ctx[hit_pipe.slot];
            ost_slot_wr_ctx.rsp_vld = 1'b1;
            ost_slot_wr_ctx.ok = hit_pipe.ok;
            ost_slot_wr_ctx.rsp_data = hit_pipe.cmd == BTI_REQ_CMD_READ ?
                hit_pipe_read_data : 32'b0;
            slot_writer_busy = 1'b1;
        end

        if (!slot_writer_busy && rd_issue_stage_hsk) begin
            rd_issue_update = 1'b1;
            ost_slot_wr_vld = 1'b1;
            ost_slot_wr_idx = rd_issue_stage_slot;
            ost_slot_wr_ctx = rd_issue_stage_ctx;
            ost_slot_wr_ctx.req_idx = rd_issue_stage_ctx.req_idx + 1'b1;
            slot_writer_busy = 1'b1;
        end
    end

    assign hit_pipe_retire = hit_pipe_update;

    always_comb begin
        read_launch = 1'b0;
        read_all_ways = 1'b1;
        read_word_mode = 1'b0;
        read_way = lookup_rd.way;
        read_set = front_set0;
        read_word_idx = beat_idx;
        read_addr = stg_req.addr;

        if (miss_state == M_WB_READ || miss_state == M_CMO_WB_READ) begin
            read_launch = 1'b1;
            read_all_ways = 1'b0;
            read_word_mode = 1'b1;
            read_way = active_way;
            read_set = active_set;
        end else if (miss_state == M_FLUSH_WB_READ) begin
            read_launch = 1'b1;
            read_all_ways = 1'b0;
            read_word_mode = 1'b1;
            read_way = flush_way;
            read_set = flush_set;
        end else if (miss_state == M_SERVE_READ) begin
            read_launch = 1'b1;
            read_all_ways = 1'b0;
            read_way = active_way;
            read_set = active_set;
            read_addr = active_cur_addr;
        end else if (accept_fast && front_is_read) begin
            read_launch = 1'b1;
        end
    end

    always_comb begin
        write_launch = 1'b0;
        write_word_mode = 1'b0;
        write_way = active_way;
        write_set = active_set;
        write_word_idx = beat_idx;
        write_addr = active_cur_addr;
        write_data = active_req.data;
        write_strobe = active_req.strobe;
        write_byte_base = active_byte_idx;
        write_byte_count = active_fragment_bytes;

        if (miss_state == M_REFILL_R && r_hsk) begin
            write_launch = 1'b1;
            write_word_mode = 1'b1;
            write_data = mem_axi4_r_slv.pkt.data;
            write_strobe = 4'hf;
        end else if (miss_state == M_SERVE_WRITE) begin
            write_launch = 1'b1;
        end else if (hit_pipe_vld &&
            hit_pipe.cmd == BTI_REQ_CMD_WRITE) begin
            write_launch = hit_pipe_update;
            write_way = hit_pipe.way;
            write_set = req_set(hit_pipe.addr);
            write_addr = hit_pipe.addr;
            write_data = hit_pipe.data;
            write_strobe = hit_pipe.strobe;
            write_byte_base = 3'b0;
            write_byte_count = req_byte_num(hit_pipe.size);
        end
    end

    assign read_base_addr = {read_set,
        read_word_mode ? read_word_idx : read_addr[2 +: WORD_IDX_W]};
    assign write_base_addr = {write_set,
        write_word_mode ? write_word_idx : write_addr[2 +: WORD_IDX_W]};

    always_comb begin
        for (int unsigned w = 0; w < WAY_NUM; w++) begin
            for (int unsigned b = 0; b < BANK_NUM; b++) begin
                data_r_cs[w][b] = 1'b0;
                data_r_addr[w][b] = read_base_addr;
                if (!read_word_mode && b < read_addr[1:0])
                    data_r_addr[w][b] = read_base_addr + 1'b1;
                data_w_cs[w][b] = 1'b0;
                data_w_addr[w][b] = '0;
                data_wdata[w][b] = 8'b0;
            end
        end

        if (read_launch) begin
            for (int unsigned w = 0; w < WAY_NUM; w++) begin
                for (int unsigned b = 0; b < BANK_NUM; b++) begin
                    if (read_all_ways || read_way == way_idx(w))
                        data_r_cs[w][b] = 1'b1;
                end
            end
        end

        if (write_launch) begin
            if (write_word_mode) begin
                for (int unsigned b = 0; b < BANK_NUM; b++) begin
                    data_w_cs[write_way][b] = write_strobe[b];
                    data_w_addr[write_way][b] = write_base_addr;
                    data_wdata[write_way][b] = write_data[b * 8 +: 8];
                end
            end else begin
                for (int unsigned j = 0; j < BANK_NUM; j++) begin
                    if (j < write_byte_count &&
                        write_strobe[2'(write_byte_base + 3'(j))]) begin
                        data_w_cs[write_way]
                            [write_addr[1:0] + 2'(j)] = 1'b1;
                        data_w_addr[write_way]
                            [write_addr[1:0] + 2'(j)] = write_base_addr +
                            (({1'b0, write_addr[1:0]} + 3'(j)) >= 3'd4);
                        data_wdata[write_way]
                            [write_addr[1:0] + 2'(j)] = write_data[
                                (write_byte_base + 3'(j)) * 8 +: 8];
                    end
                end
            end
        end
    end

    always_comb begin
        hit_pipe_read_data = 32'b0;
        active_read_data = 32'b0;
        wb_read_data = 32'b0;
        for (int unsigned j = 0; j < BANK_NUM; j++) begin
            logic [1:0] hit_bank;
            logic [1:0] active_bank;
            hit_bank = hit_pipe.addr[1:0] + 2'(j);
            active_bank = active_cur_addr[1:0] + 2'(j);
            hit_pipe_read_data[j * 8 +: 8] =
                data_bank_rdata[hit_pipe.way][hit_bank];
            active_read_data[j * 8 +: 8] =
                data_bank_rdata[active_way][active_bank];
        end
        for (int unsigned b = 0; b < BANK_NUM; b++)
            wb_read_data[b * 8 +: 8] =
                data_bank_rdata[miss_state == M_FLUSH_WB_SEND ?
                    flush_way : active_way][b];

        unique case (hit_pipe.size)
        BTI_REQ_SIZE_B1: hit_pipe_read_data[31:8] = '0;
        BTI_REQ_SIZE_B2: hit_pipe_read_data[31:16] = '0;
        default: ;
        endcase
    end

    always_comb begin
        mem_axi4_ar_mst.vld = 1'b0;
        mem_axi4_ar_mst.pkt = '0;
        mem_axi4_ar_mst.pkt.size = AXI4_AR_SIZE_B4;
        mem_axi4_ar_mst.pkt.burst = AXI4_AR_BURST_INCR;

        if (miss_state == M_CMO_L2_AR) begin
            mem_axi4_ar_mst.vld = 1'b1;
            mem_axi4_ar_mst.pkt.id = 8'b0;
            mem_axi4_ar_mst.pkt.addr = active_line_addr;
            mem_axi4_ar_mst.pkt.len = 8'b0;
            mem_axi4_ar_mst.pkt.burst = AXI4_AR_BURST_FIXED;
            mem_axi4_ar_mst.pkt.cache = 4'hf;
            mem_axi4_ar_mst.pkt.user = cmo_axi_user(active_req.cmd);
        end else if (miss_state == M_REFILL_AR) begin
            mem_axi4_ar_mst.vld = 1'b1;
            mem_axi4_ar_mst.pkt.id = 8'b0;
            mem_axi4_ar_mst.pkt.addr = active_line_addr;
            mem_axi4_ar_mst.pkt.len = AXI_LINE_LEN;
            mem_axi4_ar_mst.pkt.cache = 4'hf;
        end else if (rd_issue_stage_vld && miss_state == M_IDLE) begin
            mem_axi4_ar_mst.vld = 1'b1;
            mem_axi4_ar_mst.pkt.id = 8'(rd_issue_stage_slot);
            mem_axi4_ar_mst.pkt.addr = bypass_beat_addr(rd_issue_stage_ctx);
        end else if (ar_sel_front_bypass) begin
            mem_axi4_ar_mst.vld = 1'b1;
            mem_axi4_ar_mst.pkt.id = 8'(ost_alloc_slot);
            mem_axi4_ar_mst.pkt.addr = {stg_req.addr[31:2], 2'b00};
        end
    end

    assign rd_issue_stage_hsk = rd_issue_stage_vld &&
        miss_state == M_IDLE && ar_hsk;

    always_comb begin
        mem_axi4_aw_mst.vld = 1'b0;
        mem_axi4_aw_mst.pkt = '0;
        mem_axi4_aw_mst.pkt.size = AXI4_AW_SIZE_B4;
        mem_axi4_aw_mst.pkt.burst = AXI4_AW_BURST_INCR;
        mem_axi4_w_mst.vld = 1'b0;
        mem_axi4_w_mst.pkt = '0;

        if (miss_state == M_WB_AW || miss_state == M_CMO_WB_AW ||
            miss_state == M_FLUSH_WB_AW) begin
            mem_axi4_aw_mst.vld = 1'b1;
            mem_axi4_aw_mst.pkt.id = 8'b0;
            mem_axi4_aw_mst.pkt.addr = wb_line_addr;
            mem_axi4_aw_mst.pkt.len = AXI_LINE_LEN;
            mem_axi4_aw_mst.pkt.cache = 4'hf;
        end else if (wr_issue_vld && !wr_issue_aw_done) begin
            mem_axi4_aw_mst.vld = 1'b1;
            mem_axi4_aw_mst.pkt.id = 8'(wr_issue_slot);
            mem_axi4_aw_mst.pkt.addr = bypass_beat_addr(wr_issue_ctx);
        end

        if (miss_state == M_WB_SEND || miss_state == M_CMO_WB_SEND ||
            miss_state == M_FLUSH_WB_SEND) begin
            mem_axi4_w_mst.vld = 1'b1;
            mem_axi4_w_mst.pkt.data = wb_read_data;
            mem_axi4_w_mst.pkt.strb = 4'hf;
            mem_axi4_w_mst.pkt.last = beat_idx == LAST_BEAT;
        end else if (wr_issue_vld && !wr_issue_w_done) begin
            mem_axi4_w_mst.vld = 1'b1;
            mem_axi4_w_mst.pkt.data = bypass_write_data(wr_issue_ctx);
            mem_axi4_w_mst.pkt.strb = bypass_write_strobe(wr_issue_ctx);
            mem_axi4_w_mst.pkt.last = 1'b1;
        end
    end

    assign host_bti_req_slv.rdy = stg_wr_rdy;
    assign host_bti_rsp_mst.vld = ost_head_vld && ost_head_ctx.rsp_vld;
    assign host_bti_rsp_mst.pkt.trans_id = ost_head_ctx.trans_id;
    assign host_bti_rsp_mst.pkt.data = ost_head_ctx.rsp_data;
    assign host_bti_rsp_mst.pkt.ok = ost_head_ctx.ok;
    assign ost_free_head = host_rsp_hsk;
    assign flush_ack_mst.vld = miss_state == M_FLUSH_ACK;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            miss_state <= M_IDLE;
            active_req <= '0;
            active_slot <= '0;
            active_byte_idx <= '0;
            active_rsp_data <= '0;
            active_ok <= 1'b1;
            active_precounted_hit <= 1'b0;
            active_set <= '0;
            active_tag <= '0;
            active_way <= '0;
            active_way_vld <= 1'b0;
            active_line_addr <= '0;
            wb_line_addr <= '0;
            beat_idx <= '0;
            refill_ok <= 1'b1;
            flush_set <= '0;
            flush_way <= '0;
            flush_pending <= 1'b0;
            hit_pipe_vld <= 1'b0;
            hit_pipe <= '0;
            rd_issue_stage_vld <= 1'b0;
            rd_issue_stage_slot <= '0;
            rd_issue_stage_ctx <= '0;
            wr_issue_vld <= 1'b0;
            wr_issue_slot <= '0;
            wr_issue_ctx <= '0;
            wr_issue_aw_done <= 1'b0;
            wr_issue_w_done <= 1'b0;
            for (int unsigned w = 0; w < WAY_NUM; w++) begin
                for (int unsigned s = 0; s < SET_NUM; s++) begin
                    tag_ram[w][s] <= '0;
                    valid_ram[w][s] <= 1'b0;
                    dirty_ram[w][s] <= 1'b0;
                end
            end
            for (int unsigned s = 0; s < SET_NUM; s++)
                replace_way[s] <= '0;
        end else begin
            if (flush_slv.vld && !flush_active)
                flush_pending <= 1'b1;

            if (hit_pipe_update)
                hit_pipe_vld <= 1'b0;

            if (rd_issue_stage_hsk)
                rd_issue_stage_vld <= 1'b0;
            if (ar_sel_rd_issue) begin
                rd_issue_stage_vld <= 1'b1;
                rd_issue_stage_slot <= rd_issue_slot;
                rd_issue_stage_ctx <= rd_issue_ctx;
            end

            if (accept_fast) begin
                hit_pipe_vld <= 1'b1;
                hit_pipe.slot <= ost_alloc_slot;
                hit_pipe.addr <= stg_req.addr;
                hit_pipe.cmd <= stg_req.cmd;
                hit_pipe.size <= stg_req.size;
                hit_pipe.data <= stg_req.data;
                hit_pipe.strobe <= stg_req.strobe;
                hit_pipe.way <= lookup_rd.way;
                hit_pipe.ok <= 1'b1;
                if (front_is_write)
                    dirty_ram[lookup_rd.way][front_set0] <= 1'b1;
            end

            if (wr_issue_vld) begin
                if (aw_hsk)
                    wr_issue_aw_done <= 1'b1;
                if (w_hsk)
                    wr_issue_w_done <= 1'b1;
                if (wr_issue_update) begin
                    wr_issue_vld <= 1'b0;
                    wr_issue_aw_done <= 1'b0;
                    wr_issue_w_done <= 1'b0;
                end
            end else if (wr_load_found && miss_state == M_IDLE) begin
                wr_issue_vld <= 1'b1;
                wr_issue_slot <= wr_load_slot;
                wr_issue_ctx <= wr_load_ctx;
                wr_issue_aw_done <= 1'b0;
                wr_issue_w_done <= 1'b0;
            end else if (accept_bypass_wr) begin
                wr_issue_vld <= 1'b1;
                wr_issue_slot <= ost_alloc_slot;
                wr_issue_ctx <= ost_alloc_ctx;
                wr_issue_aw_done <= 1'b0;
                wr_issue_w_done <= 1'b0;
            end

            unique case (miss_state)
            M_IDLE: begin
                active_way_vld <= 1'b0;
                if (flush_start) begin
                    flush_pending <= 1'b0;
                    flush_set <= '0;
                    flush_way <= '0;
                    miss_state <= M_FLUSH_CHECK;
                end else if (accept_active) begin
                    active_req <= stg_req;
                    active_slot <= ost_alloc_slot;
                    active_byte_idx <= '0;
                    active_rsp_data <= '0;
                    active_ok <= 1'b1;
                    active_precounted_hit <= 1'b0;
                    miss_state <= req_is_cmo(stg_req.cmd) ?
                        M_CMO_LOOKUP : M_LOOKUP;
                end
            end
            M_LOOKUP: begin
                active_set <= active_cur_set;
                active_tag <= active_cur_tag;
                active_line_addr <= active_cur_line;
                active_way_vld <= 1'b1;
                if (active_hit) begin
                    active_way <= active_hit_way;
                    miss_state <= active_req.cmd == BTI_REQ_CMD_READ ?
                        M_SERVE_READ : M_SERVE_WRITE;
                end else begin
                    active_way <= active_victim_way;
                    if (!active_found_invalid)
                        replace_way[active_cur_set] <=
                            active_victim_way == LAST_WAY ? '0 :
                            active_victim_way + 1'b1;
                    if (active_victim_dirty) begin
                        wb_line_addr <= {active_victim_tag, active_cur_set,
                            {LINE_OFF_W{1'b0}}};
                        beat_idx <= '0;
                        miss_state <= M_WB_AW;
                    end else begin
                        beat_idx <= '0;
                        miss_state <= M_REFILL_AR;
                    end
                end
            end
            M_WB_AW: begin
                if (aw_hsk) begin
                    beat_idx <= '0;
                    miss_state <= M_WB_READ;
                end
            end
            M_WB_READ: miss_state <= M_WB_SEND;
            M_WB_SEND: begin
                if (w_hsk) begin
                    if (beat_idx == LAST_BEAT) begin
                        miss_state <= M_WB_B;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        miss_state <= M_WB_READ;
                    end
                end
            end
            M_WB_B: begin
                if (b_hsk) begin
                    dirty_ram[active_way][active_set] <= 1'b0;
                    beat_idx <= '0;
                    if (axi_resp_ok(mem_axi4_b_slv.pkt.resp)) begin
                        miss_state <= M_REFILL_AR;
                    end else begin
                        valid_ram[active_way][active_set] <= 1'b0;
                        active_ok <= 1'b0;
                        miss_state <= M_COMPLETE;
                    end
                end
            end
            M_REFILL_AR: begin
                if (ar_hsk) begin
                    beat_idx <= '0;
                    refill_ok <= 1'b1;
                    miss_state <= M_REFILL_R;
                end
            end
            M_REFILL_R: begin
                if (r_hsk) begin
                    refill_ok <= refill_ok &&
                        axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                    if (mem_axi4_r_slv.pkt.last || beat_idx == LAST_BEAT) begin
                        tag_ram[active_way][active_set] <= active_tag;
                        valid_ram[active_way][active_set] <= refill_ok &&
                            axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                        dirty_ram[active_way][active_set] <= 1'b0;
                        if (refill_ok && axi_resp_ok(mem_axi4_r_slv.pkt.resp)) begin
                            miss_state <= active_req.cmd == BTI_REQ_CMD_READ ?
                                M_SERVE_READ : M_SERVE_WRITE;
                        end else begin
                            active_ok <= 1'b0;
                            miss_state <= M_COMPLETE;
                        end
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                    end
                end
            end
            M_SERVE_READ: miss_state <= M_SERVE_USE;
            M_SERVE_USE: begin
                active_rsp_data <= active_rsp_data |
                    ((active_read_data & byte_mask(active_fragment_bytes)) <<
                        ({3'b0, active_byte_idx} << 3));
                if (active_fragment_last) begin
                    miss_state <= M_COMPLETE;
                end else begin
                    active_byte_idx <= active_byte_idx + active_fragment_bytes;
                    active_way_vld <= 1'b0;
                    miss_state <= M_LOOKUP;
                end
            end
            M_SERVE_WRITE: begin
                dirty_ram[active_way][active_set] <= 1'b1;
                if (active_fragment_last) begin
                    miss_state <= M_COMPLETE;
                end else begin
                    active_byte_idx <= active_byte_idx + active_fragment_bytes;
                    active_way_vld <= 1'b0;
                    miss_state <= M_LOOKUP;
                end
            end
            M_COMPLETE: begin
                if (ost_slot_wr_vld && ost_slot_wr_idx == active_slot) begin
                    active_way_vld <= 1'b0;
                    miss_state <= M_IDLE;
                end
            end
            M_CMO_LOOKUP: begin
                active_set <= active_cur_set;
                active_tag <= active_cur_tag;
                active_line_addr <= active_cur_line;
                if (active_hit) begin
                    active_way <= active_hit_way;
                    active_way_vld <= 1'b1;
                    if (cmo_cleans(active_req.cmd) &&
                        dirty_ram[active_hit_way][active_cur_set] && !RO) begin
                        wb_line_addr <= active_cur_line;
                        beat_idx <= '0;
                        miss_state <= M_CMO_WB_AW;
                    end else begin
                        dirty_ram[active_hit_way][active_cur_set] <= 1'b0;
                        if (cmo_invalidates(active_req.cmd))
                            valid_ram[active_hit_way][active_cur_set] <= 1'b0;
                        miss_state <= M_CMO_L2_AR;
                    end
                end else begin
                    active_way_vld <= 1'b0;
                    miss_state <= M_CMO_L2_AR;
                end
            end
            M_CMO_WB_AW: begin
                if (aw_hsk) begin
                    beat_idx <= '0;
                    miss_state <= M_CMO_WB_READ;
                end
            end
            M_CMO_WB_READ: miss_state <= M_CMO_WB_SEND;
            M_CMO_WB_SEND: begin
                if (w_hsk) begin
                    if (beat_idx == LAST_BEAT) begin
                        miss_state <= M_CMO_WB_B;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        miss_state <= M_CMO_WB_READ;
                    end
                end
            end
            M_CMO_WB_B: begin
                if (b_hsk) begin
                    if (axi_resp_ok(mem_axi4_b_slv.pkt.resp)) begin
                        dirty_ram[active_way][active_set] <= 1'b0;
                        if (cmo_invalidates(active_req.cmd))
                            valid_ram[active_way][active_set] <= 1'b0;
                        miss_state <= M_CMO_L2_AR;
                    end else begin
                        active_ok <= 1'b0;
                        miss_state <= M_COMPLETE;
                    end
                end
            end
            M_CMO_L2_AR: begin
                if (ar_hsk)
                    miss_state <= M_CMO_L2_R;
            end
            M_CMO_L2_R: begin
                if (r_hsk) begin
                    active_ok <= axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                    miss_state <= M_COMPLETE;
                end
            end
            M_FLUSH_CHECK: begin
                if (valid_ram[flush_way][flush_set] &&
                    dirty_ram[flush_way][flush_set] && !RO) begin
                    wb_line_addr <= {tag_ram[flush_way][flush_set],
                        flush_set, {LINE_OFF_W{1'b0}}};
                    beat_idx <= '0;
                    miss_state <= M_FLUSH_WB_AW;
                end else begin
                    valid_ram[flush_way][flush_set] <= 1'b0;
                    dirty_ram[flush_way][flush_set] <= 1'b0;
                    if (flush_way == LAST_WAY && flush_set == LAST_SET) begin
                        miss_state <= M_FLUSH_ACK;
                    end else if (flush_way == LAST_WAY) begin
                        flush_way <= '0;
                        flush_set <= flush_set + 1'b1;
                    end else begin
                        flush_way <= flush_way + 1'b1;
                    end
                end
            end
            M_FLUSH_WB_AW: begin
                if (aw_hsk) begin
                    beat_idx <= '0;
                    miss_state <= M_FLUSH_WB_READ;
                end
            end
            M_FLUSH_WB_READ: miss_state <= M_FLUSH_WB_SEND;
            M_FLUSH_WB_SEND: begin
                if (w_hsk) begin
                    if (beat_idx == LAST_BEAT) begin
                        miss_state <= M_FLUSH_WB_B;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        miss_state <= M_FLUSH_WB_READ;
                    end
                end
            end
            M_FLUSH_WB_B: begin
                if (b_hsk) begin
                    valid_ram[flush_way][flush_set] <= 1'b0;
                    dirty_ram[flush_way][flush_set] <= 1'b0;
                    if (flush_way == LAST_WAY && flush_set == LAST_SET) begin
                        miss_state <= M_FLUSH_ACK;
                    end else if (flush_way == LAST_WAY) begin
                        flush_way <= '0;
                        flush_set <= flush_set + 1'b1;
                        miss_state <= M_FLUSH_CHECK;
                    end else begin
                        flush_way <= flush_way + 1'b1;
                        miss_state <= M_FLUSH_CHECK;
                    end
                end
            end
            M_FLUSH_ACK: begin
                miss_state <= M_IDLE;
            end
            default: miss_state <= M_IDLE;
            endcase
        end
    end

    assign perf_mst.pkt.hit = accept_fast ||
        (miss_state == M_LOOKUP && !active_precounted_hit && active_hit);
    assign perf_mst.pkt.miss = miss_state == M_LOOKUP && !active_hit;
    assign perf_mst.pkt.bypass = accept_bypass_rd || accept_bypass_wr ||
        (accept_ro_error && front_is_bypass);
    assign perf_mst.pkt.writeback = aw_hsk &&
        (miss_state == M_WB_AW || miss_state == M_CMO_WB_AW ||
            miss_state == M_FLUSH_WB_AW);
    assign perf_mst.pkt.stg_full = host_bti_req_slv.vld &&
        !host_bti_req_slv.rdy;
    assign perf_mst.pkt.ost_full = stg_rd_vld && !flush_active && ost_full;
    assign perf_mst.pkt.miss_busy = stg_rd_vld && !flush_pending &&
        !flush_active && !ost_full && !front_is_bypass &&
        !(RO && front_is_write) && !front_fast_hit &&
        (miss_state != M_IDLE || has_bypass_ost);

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && mem_axi4_r_slv.vld && miss_state != M_REFILL_R &&
            miss_state != M_CMO_L2_R) begin
            assert (mem_axi4_r_slv.pkt.id < 8'(OST_DEPTH))
                else $fatal(1, "L1 AXI R id outside OST range");
        end
        if (rst_n && mem_axi4_b_slv.vld &&
            miss_state != M_WB_B && miss_state != M_CMO_WB_B &&
            miss_state != M_FLUSH_WB_B) begin
            assert (mem_axi4_b_slv.pkt.id < 8'(OST_DEPTH))
                else $fatal(1, "L1 AXI B id outside OST range");
        end
    end
`endif
endmodule
