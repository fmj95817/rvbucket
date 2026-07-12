`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"
`include "spec/core/core.svh"

module l2 #(
    parameter int SIZE = `L2_SIZE,
    parameter int WAY_NUM = `L2_WAY_NUM,
    parameter int LINE_SIZE = `L2_LINE_SIZE
)(
    input logic       clk,
    input logic       rst_n,
    axi4_aw_if_t.slv  i_axi4_aw_slv,
    axi4_w_if_t.slv   i_axi4_w_slv,
    axi4_b_if_t.mst   i_axi4_b_mst,
    axi4_ar_if_t.slv  i_axi4_ar_slv,
    axi4_r_if_t.mst   i_axi4_r_mst,
    axi4_aw_if_t.slv  d_axi4_aw_slv,
    axi4_w_if_t.slv   d_axi4_w_slv,
    axi4_b_if_t.mst   d_axi4_b_mst,
    axi4_ar_if_t.slv  d_axi4_ar_slv,
    axi4_r_if_t.mst   d_axi4_r_mst,
    axi4_aw_if_t.mst  mem_axi4_aw_mst,
    axi4_w_if_t.mst   mem_axi4_w_mst,
    axi4_b_if_t.slv   mem_axi4_b_slv,
    axi4_ar_if_t.mst  mem_axi4_ar_mst,
    axi4_r_if_t.slv   mem_axi4_r_slv
);
    localparam int WORD_SIZE = 4;
    localparam int WORD_NUM = LINE_SIZE / WORD_SIZE;
    localparam int WORD_IDX_W = $clog2(WORD_NUM);
    localparam int LINE_OFF_W = $clog2(LINE_SIZE);
    localparam int SET_NUM = SIZE / (WAY_NUM * LINE_SIZE);
    localparam int SET_W = $clog2(SET_NUM);
    localparam int WAY_W = WAY_NUM <= 1 ? 1 : $clog2(WAY_NUM);
    localparam int TAG_W = 32 - LINE_OFF_W - SET_W;
    localparam int META_DW = TAG_W + 2;
    localparam int DATA_AW = SET_W + WORD_IDX_W;
    localparam logic [WORD_IDX_W-1:0] LAST_BEAT = WORD_IDX_W'(WORD_NUM - 1);
    localparam logic [WAY_W-1:0] LAST_WAY = WAY_W'(WAY_NUM - 1);
    localparam logic [7:0] AXI_LINE_LEN = 8'(WORD_NUM - 1);

    typedef enum logic [4:0] {
        S_INIT,
        S_IDLE,
        S_META_READ,
        S_LOOKUP,
        S_RD_READ,
        S_RD_SEND,
        S_WR_WAIT_W,
        S_WR_WRITE,
        S_WR_B,
        S_WB_AW,
        S_WB_READ,
        S_WB_CAPTURE,
        S_WB_W,
        S_WB_B,
        S_REFILL_AR,
        S_REFILL_R,
        S_BYPASS_AR,
        S_BYPASS_R,
        S_BYPASS_AW,
        S_BYPASS_W,
        S_BYPASS_B
    } state_t;

    state_t state;
    logic rr_d;

    logic idle_sel_vld;
    logic idle_sel_port;
    logic idle_sel_write;

    logic req_port;
    logic req_write;
    logic req_cacheable;
    logic [7:0] req_id;
    logic [31:0] req_addr;
    logic [7:0] req_len;
    logic [2:0] req_size;
    logic [1:0] req_burst;
    logic req_lock;
    logic [3:0] req_cache;
    logic [2:0] req_prot;
    logic [3:0] req_qos;
    logic [31:0] req_user;
    logic [7:0] req_beat_idx;

    logic [31:0] wr_data;
    logic [3:0] wr_strb;
    logic wr_last;

    logic [SET_W-1:0] req_set;
    logic [WAY_W-1:0] req_way;
    logic [TAG_W-1:0] req_tag;
    logic [31:0] req_line_addr;
    logic [31:0] wb_line_addr;
    logic [WORD_IDX_W-1:0] beat_idx;
    logic refill_ok;
    logic [31:0] wb_word_data;
    axi4_r_resp_t rsp_r_resp;
    axi4_b_resp_t rsp_b_resp;
    logic [SET_W-1:0] init_set;

    logic [WAY_W-1:0] replace_way[SET_NUM];

    sram_rw_if_t #(SET_W, META_DW) meta_rw_if[WAY_NUM]();
    sram_rw_if_t #(DATA_AW, 32) data_rw_if[WAY_NUM]();
    logic meta_cs;
    logic meta_wen;
    logic [WAY_W-1:0] meta_way;
    logic [SET_W-1:0] meta_addr;
    logic [META_DW-1:0] meta_wdata;
    logic [META_DW-1:0] way_meta_rdata[WAY_NUM];
    logic [TAG_W-1:0] way_meta_tag[WAY_NUM];
    logic way_meta_valid[WAY_NUM];
    logic way_meta_dirty[WAY_NUM];
    logic data_cs;
    logic data_wen;
    logic [WAY_W-1:0] data_way;
    logic [DATA_AW-1:0] data_addr;
    logic [31:0] data_wdata;
    logic [31:0] way_rdata[WAY_NUM];
    logic [31:0] data_rdata;

    logic hit;
    logic [WAY_W-1:0] hit_way;
    logic found_invalid;
    logic [WAY_W-1:0] victim_way;
    logic victim_dirty;
    logic [TAG_W-1:0] victim_tag;

    wire i_req_vld = i_axi4_aw_slv.vld || i_axi4_ar_slv.vld;
    wire d_req_vld = d_axi4_aw_slv.vld || d_axi4_ar_slv.vld;
    wire sel_w_vld = req_port ? d_axi4_w_slv.vld : i_axi4_w_slv.vld;
    wire [31:0] sel_w_data = req_port ?
        d_axi4_w_slv.pkt.data : i_axi4_w_slv.pkt.data;
    wire [3:0] sel_w_strb = req_port ?
        d_axi4_w_slv.pkt.strb : i_axi4_w_slv.pkt.strb;
    wire sel_w_last = req_port ?
        d_axi4_w_slv.pkt.last : i_axi4_w_slv.pkt.last;

    wire aw_hsk = mem_axi4_aw_mst.vld && mem_axi4_aw_mst.rdy;
    wire w_hsk = mem_axi4_w_mst.vld && mem_axi4_w_mst.rdy;
    wire b_hsk = mem_axi4_b_slv.vld && mem_axi4_b_slv.rdy;
    wire ar_hsk = mem_axi4_ar_mst.vld && mem_axi4_ar_mst.rdy;
    wire r_hsk = mem_axi4_r_slv.vld && mem_axi4_r_slv.rdy;
    wire host_r_hsk = (i_axi4_r_mst.vld && i_axi4_r_mst.rdy) ||
        (d_axi4_r_mst.vld && d_axi4_r_mst.rdy);
    wire host_b_hsk = (i_axi4_b_mst.vld && i_axi4_b_mst.rdy) ||
        (d_axi4_b_mst.vld && d_axi4_b_mst.rdy);

    wire [SET_W-1:0] cur_set = req_addr[LINE_OFF_W +: SET_W];
    wire [TAG_W-1:0] cur_tag = req_addr[31 -: TAG_W];
    wire [WORD_IDX_W-1:0] cur_word_idx = req_addr[2 +: WORD_IDX_W];
    wire [DATA_AW-1:0] cur_data_addr = {req_set, cur_word_idx};
    wire [DATA_AW-1:0] beat_data_addr = {req_set, beat_idx};
    wire [31:0] cur_line_addr = {
        req_addr[31:LINE_OFF_W],
        {LINE_OFF_W{1'b0}}
    };
    wire [31:0] victim_line_addr = {
        victim_tag,
        req_set,
        {LINE_OFF_W{1'b0}}
    };
    wire req_last_beat = req_beat_idx == req_len;

    function automatic logic [WAY_W-1:0] way_idx(input int unsigned idx);
        way_idx = idx[WAY_W-1:0];
    endfunction

    function automatic logic [31:0] axi_next_addr(
        input logic [31:0] addr,
        input logic [2:0] size,
        input logic [1:0] burst
    );
        unique case (burst)
        2'd0: axi_next_addr = addr;
        default: axi_next_addr = addr + (32'd1 << size);
        endcase
    endfunction

    function automatic logic [31:0] merge_word(
        input logic [31:0] old_word,
        input logic [31:0] new_word,
        input logic [3:0] strb
    );
        logic [31:0] merged;
        begin
            merged = old_word;
            for (int unsigned i = 0; i < WORD_SIZE; i++) begin
                if (strb[i])
                    merged[i * 8 +: 8] = new_word[i * 8 +: 8];
            end
            merge_word = merged;
        end
    endfunction

    function automatic logic axi_resp_ok(input logic [1:0] resp);
        axi_resp_ok = resp == 2'b00;
    endfunction

    genvar way;
    generate
        for (way = 0; way < WAY_NUM; way++) begin : gen_data_way
            localparam logic [WAY_W-1:0] WAY_IDX = way_idx(way);

            assign meta_rw_if[way].cs = meta_cs &&
                (!meta_wen || state == S_INIT || meta_way == WAY_IDX);
            assign meta_rw_if[way].addr = meta_addr;
            assign meta_rw_if[way].wen = meta_wen;
            assign meta_rw_if[way].wdata = meta_wdata;
            assign way_meta_rdata[way] = meta_rw_if[way].rdata;
            assign way_meta_dirty[way] = way_meta_rdata[way][0];
            assign way_meta_valid[way] = way_meta_rdata[way][1];
            assign way_meta_tag[way] = way_meta_rdata[way][META_DW-1:2];

            assign data_rw_if[way].cs = data_cs && data_way == WAY_IDX;
            assign data_rw_if[way].addr = data_addr;
            assign data_rw_if[way].wen = data_wen;
            assign data_rw_if[way].wdata = data_wdata;
            assign way_rdata[way] = data_rw_if[way].rdata;

            sram #(
                .AW (SET_W),
                .DW (META_DW)
            ) u_meta_sram(
                .clk         (clk),
                .sram_rw_slv (meta_rw_if[way])
            );

            sram #(
                .AW (DATA_AW),
                .DW (32)
            ) u_data_sram(
                .clk         (clk),
                .sram_rw_slv (data_rw_if[way])
            );
        end
    endgenerate

    always_comb begin
        idle_sel_vld = 1'b0;
        idle_sel_port = 1'b0;
        idle_sel_write = 1'b0;

        if (rr_d) begin
            if (d_axi4_aw_slv.vld) begin
                idle_sel_vld = 1'b1;
                idle_sel_port = 1'b1;
                idle_sel_write = 1'b1;
            end else if (d_axi4_ar_slv.vld) begin
                idle_sel_vld = 1'b1;
                idle_sel_port = 1'b1;
            end else if (i_axi4_aw_slv.vld) begin
                idle_sel_vld = 1'b1;
                idle_sel_write = 1'b1;
            end else if (i_axi4_ar_slv.vld) begin
                idle_sel_vld = 1'b1;
            end
        end else begin
            if (i_axi4_aw_slv.vld) begin
                idle_sel_vld = 1'b1;
                idle_sel_write = 1'b1;
            end else if (i_axi4_ar_slv.vld) begin
                idle_sel_vld = 1'b1;
            end else if (d_axi4_aw_slv.vld) begin
                idle_sel_vld = 1'b1;
                idle_sel_port = 1'b1;
                idle_sel_write = 1'b1;
            end else if (d_axi4_ar_slv.vld) begin
                idle_sel_vld = 1'b1;
                idle_sel_port = 1'b1;
            end
        end
    end

    always_comb begin
        data_rdata = 32'b0;
        for (int unsigned i = 0; i < WAY_NUM; i++) begin
            if (data_way == way_idx(i))
                data_rdata = way_rdata[i];
        end
    end

    always_comb begin
        hit = 1'b0;
        hit_way = '0;
        for (int unsigned i = 0; i < WAY_NUM; i++) begin
            if (!hit && way_meta_valid[i] && way_meta_tag[i] == req_tag) begin
                hit = 1'b1;
                hit_way = way_idx(i);
            end
        end
    end

    always_comb begin
        found_invalid = 1'b0;
        victim_way = replace_way[req_set];
        for (int unsigned i = 0; i < WAY_NUM; i++) begin
            if (!found_invalid && !way_meta_valid[i]) begin
                found_invalid = 1'b1;
                victim_way = way_idx(i);
            end
        end
        victim_dirty = way_meta_valid[victim_way] && way_meta_dirty[victim_way];
        victim_tag = way_meta_tag[victim_way];
    end

    always_comb begin
        meta_cs = 1'b0;
        meta_wen = 1'b0;
        meta_way = req_way;
        meta_addr = req_set;
        meta_wdata = {{TAG_W{1'b0}}, 2'b00};

        unique case (state)
        S_INIT: begin
            meta_cs = 1'b1;
            meta_wen = 1'b1;
            meta_addr = init_set;
        end
        S_META_READ: begin
            meta_cs = 1'b1;
            meta_addr = cur_set;
        end
        S_WR_WRITE: begin
            meta_cs = 1'b1;
            meta_wen = 1'b1;
            meta_wdata = {req_tag, 2'b11};
        end
        S_REFILL_R: begin
            if (r_hsk && (mem_axi4_r_slv.pkt.last || beat_idx == LAST_BEAT)) begin
                meta_cs = 1'b1;
                meta_wen = 1'b1;
                meta_wdata = {
                    req_tag,
                    refill_ok && axi_resp_ok(mem_axi4_r_slv.pkt.resp),
                    1'b0
                };
            end
        end
        S_WB_B: begin
            if (b_hsk && !axi_resp_ok(mem_axi4_b_slv.pkt.resp)) begin
                meta_cs = 1'b1;
                meta_wen = 1'b1;
            end
        end
        default: ;
        endcase
    end

    always_comb begin
        data_cs = 1'b0;
        data_wen = 1'b0;
        data_way = req_way;
        data_addr = beat_data_addr;
        data_wdata = 32'b0;

        unique case (state)
        S_RD_READ: begin
            data_cs = 1'b1;
            data_addr = cur_data_addr;
        end
        S_WR_WAIT_W: begin
            data_cs = sel_w_vld;
            data_addr = cur_data_addr;
        end
        S_WR_WRITE: begin
            data_cs = 1'b1;
            data_wen = 1'b1;
            data_addr = cur_data_addr;
            data_wdata = merge_word(data_rdata, wr_data, wr_strb);
        end
        S_WB_READ: begin
            data_cs = 1'b1;
            data_addr = beat_data_addr;
        end
        S_REFILL_R: begin
            data_cs = r_hsk;
            data_wen = r_hsk;
            data_addr = beat_data_addr;
            data_wdata = mem_axi4_r_slv.pkt.data;
        end
        default: ;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_INIT;
            rr_d <= 1'b0;
            req_port <= 1'b0;
            req_write <= 1'b0;
            req_cacheable <= 1'b0;
            req_id <= '0;
            req_addr <= '0;
            req_len <= '0;
            req_size <= '0;
            req_burst <= '0;
            req_lock <= 1'b0;
            req_cache <= '0;
            req_prot <= '0;
            req_qos <= '0;
            req_user <= '0;
            req_beat_idx <= '0;
            wr_data <= '0;
            wr_strb <= '0;
            wr_last <= 1'b0;
            req_set <= '0;
            req_way <= '0;
            req_tag <= '0;
            req_line_addr <= '0;
            wb_line_addr <= '0;
            beat_idx <= '0;
            refill_ok <= 1'b1;
            wb_word_data <= '0;
            rsp_r_resp <= AXI4_R_RESP_OKAY;
            rsp_b_resp <= AXI4_B_RESP_OKAY;
            init_set <= '0;
            for (int s = 0; s < SET_NUM; s++)
                replace_way[s] <= '0;
        end else begin
            unique case (state)
            S_INIT: begin
                if (init_set == SET_W'(SET_NUM - 1)) begin
                    state <= S_IDLE;
                end else begin
                    init_set <= init_set + 1'b1;
                end
            end
            S_IDLE: begin
                req_beat_idx <= '0;
                beat_idx <= '0;
                rsp_r_resp <= AXI4_R_RESP_OKAY;
                rsp_b_resp <= AXI4_B_RESP_OKAY;
                if (idle_sel_vld &&
                    ((idle_sel_write && ((idle_sel_port && d_axi4_aw_slv.rdy) ||
                    (!idle_sel_port && i_axi4_aw_slv.rdy))) ||
                    (!idle_sel_write && ((idle_sel_port && d_axi4_ar_slv.rdy) ||
                    (!idle_sel_port && i_axi4_ar_slv.rdy))))) begin
                    req_port <= idle_sel_port;
                    req_write <= idle_sel_write;
                    rr_d <= !idle_sel_port;
                    if (idle_sel_write) begin
                        req_cacheable <= (idle_sel_port ?
                            d_axi4_aw_slv.pkt.cache : i_axi4_aw_slv.pkt.cache) != 4'h0;
                        req_id <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.id : i_axi4_aw_slv.pkt.id;
                        req_addr <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.addr : i_axi4_aw_slv.pkt.addr;
                        req_len <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.len : i_axi4_aw_slv.pkt.len;
                        req_size <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.size : i_axi4_aw_slv.pkt.size;
                        req_burst <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.burst : i_axi4_aw_slv.pkt.burst;
                        req_lock <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.lock : i_axi4_aw_slv.pkt.lock;
                        req_cache <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.cache : i_axi4_aw_slv.pkt.cache;
                        req_prot <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.prot : i_axi4_aw_slv.pkt.prot;
                        req_qos <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.qos : i_axi4_aw_slv.pkt.qos;
                        req_user <= idle_sel_port ?
                            d_axi4_aw_slv.pkt.user : i_axi4_aw_slv.pkt.user;
                        state <= (idle_sel_port ?
                            d_axi4_aw_slv.pkt.cache : i_axi4_aw_slv.pkt.cache) == 4'h0 ?
                            S_BYPASS_AW : S_META_READ;
                    end else begin
                        req_cacheable <= (idle_sel_port ?
                            d_axi4_ar_slv.pkt.cache : i_axi4_ar_slv.pkt.cache) != 4'h0;
                        req_id <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.id : i_axi4_ar_slv.pkt.id;
                        req_addr <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.addr : i_axi4_ar_slv.pkt.addr;
                        req_len <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.len : i_axi4_ar_slv.pkt.len;
                        req_size <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.size : i_axi4_ar_slv.pkt.size;
                        req_burst <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.burst : i_axi4_ar_slv.pkt.burst;
                        req_lock <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.lock : i_axi4_ar_slv.pkt.lock;
                        req_cache <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.cache : i_axi4_ar_slv.pkt.cache;
                        req_prot <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.prot : i_axi4_ar_slv.pkt.prot;
                        req_qos <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.qos : i_axi4_ar_slv.pkt.qos;
                        req_user <= idle_sel_port ?
                            d_axi4_ar_slv.pkt.user : i_axi4_ar_slv.pkt.user;
                        state <= (idle_sel_port ?
                            d_axi4_ar_slv.pkt.cache : i_axi4_ar_slv.pkt.cache) == 4'h0 ?
                            S_BYPASS_AR : S_META_READ;
                    end
                end
            end
            S_META_READ: begin
                req_set <= cur_set;
                req_tag <= cur_tag;
                req_line_addr <= cur_line_addr;
                state <= S_LOOKUP;
            end
            S_LOOKUP: begin
                if (hit) begin
                    req_way <= hit_way;
                    state <= req_write ? S_WR_WAIT_W : S_RD_READ;
                end else begin
                    req_way <= victim_way;
                    if (!found_invalid)
                        replace_way[cur_set] <= victim_way + 1'b1;
                    if (victim_dirty) begin
                        wb_line_addr <= victim_line_addr;
                        beat_idx <= '0;
                        state <= S_WB_AW;
                    end else begin
                        beat_idx <= '0;
                        state <= S_REFILL_AR;
                    end
                end
            end
            S_RD_READ: begin
                state <= S_RD_SEND;
            end
            S_RD_SEND: begin
                if (host_r_hsk) begin
                    if (req_last_beat || rsp_r_resp != AXI4_R_RESP_OKAY) begin
                        state <= S_IDLE;
                    end else begin
                        req_beat_idx <= req_beat_idx + 1'b1;
                        req_addr <= axi_next_addr(req_addr, req_size, req_burst);
                        state <= S_META_READ;
                    end
                end
            end
            S_WR_WAIT_W: begin
                if (sel_w_vld) begin
                    wr_data <= sel_w_data;
                    wr_strb <= sel_w_strb;
                    wr_last <= sel_w_last;
                    state <= S_WR_WRITE;
                end
            end
            S_WR_WRITE: begin
                if (req_last_beat || wr_last) begin
                    state <= S_WR_B;
                end else begin
                    req_beat_idx <= req_beat_idx + 1'b1;
                    req_addr <= axi_next_addr(req_addr, req_size, req_burst);
                    state <= S_META_READ;
                end
            end
            S_WR_B: begin
                if (host_b_hsk)
                    state <= S_IDLE;
            end
            S_WB_AW: begin
                if (aw_hsk) begin
                    beat_idx <= '0;
                    state <= S_WB_READ;
                end
            end
            S_WB_READ: begin
                state <= S_WB_CAPTURE;
            end
            S_WB_CAPTURE: begin
                wb_word_data <= data_rdata;
                state <= S_WB_W;
            end
            S_WB_W: begin
                if (w_hsk) begin
                    if (beat_idx == LAST_BEAT) begin
                        state <= S_WB_B;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        state <= S_WB_READ;
                    end
                end
            end
            S_WB_B: begin
                if (b_hsk) begin
                    if (axi_resp_ok(mem_axi4_b_slv.pkt.resp)) begin
                        beat_idx <= '0;
                        state <= S_REFILL_AR;
                    end else begin
                        if (req_write) begin
                            rsp_b_resp <= AXI4_B_RESP_SLVERR;
                            state <= S_WR_B;
                        end else begin
                            rsp_r_resp <= AXI4_R_RESP_SLVERR;
                            state <= S_RD_SEND;
                        end
                    end
                end
            end
            S_REFILL_AR: begin
                if (ar_hsk) begin
                    beat_idx <= '0;
                    refill_ok <= 1'b1;
                    state <= S_REFILL_R;
                end
            end
            S_REFILL_R: begin
                if (r_hsk) begin
                    refill_ok <= refill_ok &&
                        axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                    if (mem_axi4_r_slv.pkt.last || beat_idx == LAST_BEAT) begin
                        if (refill_ok && axi_resp_ok(mem_axi4_r_slv.pkt.resp)) begin
                            state <= req_write ? S_WR_WAIT_W : S_RD_READ;
                        end else if (req_write) begin
                            rsp_b_resp <= AXI4_B_RESP_SLVERR;
                            state <= S_WR_B;
                        end else begin
                            rsp_r_resp <= AXI4_R_RESP_SLVERR;
                            state <= S_RD_SEND;
                        end
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                    end
                end
            end
            S_BYPASS_AR: begin
                if (ar_hsk)
                    state <= S_BYPASS_R;
            end
            S_BYPASS_R: begin
                if (r_hsk && mem_axi4_r_slv.pkt.last)
                    state <= S_IDLE;
            end
            S_BYPASS_AW: begin
                if (aw_hsk)
                    state <= S_BYPASS_W;
            end
            S_BYPASS_W: begin
                if (w_hsk && mem_axi4_w_mst.pkt.last)
                    state <= S_BYPASS_B;
            end
            S_BYPASS_B: begin
                if (b_hsk)
                    state <= S_IDLE;
            end
            default: begin
                state <= S_IDLE;
            end
            endcase
        end
    end

    assign i_axi4_aw_slv.rdy = state == S_IDLE && idle_sel_vld &&
        !idle_sel_port && idle_sel_write;
    assign d_axi4_aw_slv.rdy = state == S_IDLE && idle_sel_vld &&
        idle_sel_port && idle_sel_write;
    assign i_axi4_ar_slv.rdy = state == S_IDLE && idle_sel_vld &&
        !idle_sel_port && !idle_sel_write;
    assign d_axi4_ar_slv.rdy = state == S_IDLE && idle_sel_vld &&
        idle_sel_port && !idle_sel_write;
    assign i_axi4_w_slv.rdy =
        (!req_port && state == S_WR_WAIT_W) ||
        (!req_port && state == S_BYPASS_W && mem_axi4_w_mst.rdy);
    assign d_axi4_w_slv.rdy =
        (req_port && state == S_WR_WAIT_W) ||
        (req_port && state == S_BYPASS_W && mem_axi4_w_mst.rdy);

    assign i_axi4_r_mst.vld =
        (!req_port && state == S_RD_SEND) ||
        (!req_port && state == S_BYPASS_R && mem_axi4_r_slv.vld);
    assign d_axi4_r_mst.vld =
        (req_port && state == S_RD_SEND) ||
        (req_port && state == S_BYPASS_R && mem_axi4_r_slv.vld);
    assign i_axi4_r_mst.pkt.id =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.id : req_id;
    assign d_axi4_r_mst.pkt.id =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.id : req_id;
    assign i_axi4_r_mst.pkt.data =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.data :
            (rsp_r_resp == AXI4_R_RESP_OKAY ? data_rdata : 32'b0);
    assign d_axi4_r_mst.pkt.data =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.data :
            (rsp_r_resp == AXI4_R_RESP_OKAY ? data_rdata : 32'b0);
    assign i_axi4_r_mst.pkt.resp =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.resp : rsp_r_resp;
    assign d_axi4_r_mst.pkt.resp =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.resp : rsp_r_resp;
    assign i_axi4_r_mst.pkt.last =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.last :
            (req_last_beat || rsp_r_resp != AXI4_R_RESP_OKAY);
    assign d_axi4_r_mst.pkt.last =
        state == S_BYPASS_R ? mem_axi4_r_slv.pkt.last :
            (req_last_beat || rsp_r_resp != AXI4_R_RESP_OKAY);

    assign i_axi4_b_mst.vld =
        (!req_port && state == S_WR_B) ||
        (!req_port && state == S_BYPASS_B && mem_axi4_b_slv.vld);
    assign d_axi4_b_mst.vld =
        (req_port && state == S_WR_B) ||
        (req_port && state == S_BYPASS_B && mem_axi4_b_slv.vld);
    assign i_axi4_b_mst.pkt.id =
        state == S_BYPASS_B ? mem_axi4_b_slv.pkt.id : req_id;
    assign d_axi4_b_mst.pkt.id =
        state == S_BYPASS_B ? mem_axi4_b_slv.pkt.id : req_id;
    assign i_axi4_b_mst.pkt.resp =
        state == S_BYPASS_B ? mem_axi4_b_slv.pkt.resp : rsp_b_resp;
    assign d_axi4_b_mst.pkt.resp =
        state == S_BYPASS_B ? mem_axi4_b_slv.pkt.resp : rsp_b_resp;

    assign mem_axi4_ar_mst.vld = state == S_REFILL_AR || state == S_BYPASS_AR;
    assign mem_axi4_ar_mst.pkt.id = state == S_BYPASS_AR ? req_id : '0;
    assign mem_axi4_ar_mst.pkt.addr =
        state == S_BYPASS_AR ? req_addr : req_line_addr;
    assign mem_axi4_ar_mst.pkt.len =
        state == S_BYPASS_AR ? req_len : AXI_LINE_LEN;
    assign mem_axi4_ar_mst.pkt.size =
        state == S_BYPASS_AR ? axi4_ar_size_t'(req_size) : AXI4_AR_SIZE_B4;
    assign mem_axi4_ar_mst.pkt.burst =
        state == S_BYPASS_AR ? axi4_ar_burst_t'(req_burst) :
            AXI4_AR_BURST_INCR;
    assign mem_axi4_ar_mst.pkt.lock = state == S_BYPASS_AR ? req_lock : 1'b0;
    assign mem_axi4_ar_mst.pkt.cache = state == S_BYPASS_AR ? req_cache : 4'hf;
    assign mem_axi4_ar_mst.pkt.prot = state == S_BYPASS_AR ? req_prot : 3'h0;
    assign mem_axi4_ar_mst.pkt.qos = state == S_BYPASS_AR ? req_qos : 4'h0;
    assign mem_axi4_ar_mst.pkt.user = state == S_BYPASS_AR ? req_user : '0;

    assign mem_axi4_aw_mst.vld = state == S_WB_AW || state == S_BYPASS_AW;
    assign mem_axi4_aw_mst.pkt.id = state == S_BYPASS_AW ? req_id : '0;
    assign mem_axi4_aw_mst.pkt.addr =
        state == S_BYPASS_AW ? req_addr : wb_line_addr;
    assign mem_axi4_aw_mst.pkt.len =
        state == S_BYPASS_AW ? req_len : AXI_LINE_LEN;
    assign mem_axi4_aw_mst.pkt.size =
        state == S_BYPASS_AW ? axi4_aw_size_t'(req_size) : AXI4_AW_SIZE_B4;
    assign mem_axi4_aw_mst.pkt.burst =
        state == S_BYPASS_AW ? axi4_aw_burst_t'(req_burst) :
            AXI4_AW_BURST_INCR;
    assign mem_axi4_aw_mst.pkt.lock = state == S_BYPASS_AW ? req_lock : 1'b0;
    assign mem_axi4_aw_mst.pkt.cache = state == S_BYPASS_AW ? req_cache : 4'hf;
    assign mem_axi4_aw_mst.pkt.prot = state == S_BYPASS_AW ? req_prot : 3'h0;
    assign mem_axi4_aw_mst.pkt.qos = state == S_BYPASS_AW ? req_qos : 4'h0;
    assign mem_axi4_aw_mst.pkt.user = state == S_BYPASS_AW ? req_user : '0;

    assign mem_axi4_w_mst.vld =
        state == S_WB_W || (state == S_BYPASS_W && sel_w_vld);
    assign mem_axi4_w_mst.pkt.data =
        state == S_BYPASS_W ? sel_w_data : wb_word_data;
    assign mem_axi4_w_mst.pkt.strb =
        state == S_BYPASS_W ? sel_w_strb : 4'hf;
    assign mem_axi4_w_mst.pkt.last =
        state == S_BYPASS_W ? sel_w_last : beat_idx == LAST_BEAT;

    assign mem_axi4_r_slv.rdy =
        state == S_REFILL_R ||
        (state == S_BYPASS_R &&
            (req_port ? d_axi4_r_mst.rdy : i_axi4_r_mst.rdy));
    assign mem_axi4_b_slv.rdy =
        state == S_WB_B ||
        (state == S_BYPASS_B &&
            (req_port ? d_axi4_b_mst.rdy : i_axi4_b_mst.rdy));

`ifndef SYNTHESIS
    always_ff @(posedge clk) begin
        if (rst_n && state == S_IDLE && idle_sel_vld) begin
            if (idle_sel_write) begin
                if ((idle_sel_port ? d_axi4_aw_slv.pkt.cache :
                    i_axi4_aw_slv.pkt.cache) != 4'h0) begin
                    assert ((idle_sel_port ? d_axi4_aw_slv.pkt.size :
                        i_axi4_aw_slv.pkt.size) == AXI4_AW_SIZE_B4)
                        else $fatal(1, "L2 cacheable AW size must be B4");
                    assert ((idle_sel_port ? d_axi4_aw_slv.pkt.burst :
                        i_axi4_aw_slv.pkt.burst) == AXI4_AW_BURST_INCR)
                        else $fatal(1, "L2 cacheable AW burst must be INCR");
                    assert ((idle_sel_port ? d_axi4_aw_slv.pkt.addr[1:0] :
                        i_axi4_aw_slv.pkt.addr[1:0]) == 2'b00)
                        else $fatal(1, "L2 cacheable AW must be word aligned");
                end
            end else begin
                if ((idle_sel_port ? d_axi4_ar_slv.pkt.cache :
                    i_axi4_ar_slv.pkt.cache) != 4'h0) begin
                    assert ((idle_sel_port ? d_axi4_ar_slv.pkt.size :
                        i_axi4_ar_slv.pkt.size) == AXI4_AR_SIZE_B4)
                        else $fatal(1, "L2 cacheable AR size must be B4");
                    assert ((idle_sel_port ? d_axi4_ar_slv.pkt.burst :
                        i_axi4_ar_slv.pkt.burst) == AXI4_AR_BURST_INCR)
                        else $fatal(1, "L2 cacheable AR burst must be INCR");
                    assert ((idle_sel_port ? d_axi4_ar_slv.pkt.addr[1:0] :
                        i_axi4_ar_slv.pkt.addr[1:0]) == 2'b00)
                        else $fatal(1, "L2 cacheable AR must be word aligned");
                end
            end
        end

        if (rst_n && state == S_WR_WAIT_W && sel_w_vld) begin
            assert (sel_w_last == req_last_beat)
                else $fatal(1, "L2 W last does not match AW len");
        end
    end
`endif
endmodule
