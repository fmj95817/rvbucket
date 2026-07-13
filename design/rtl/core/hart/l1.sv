`include "itf/bti_req_if.svh"
`include "itf/axi4_aw_if.svh"
`include "itf/axi4_ar_if.svh"
`include "itf/axi4_b_if.svh"
`include "itf/axi4_r_if.svh"
`include "spec/core/hart.svh"

module l1 #(
    parameter bit RO = 1'b0,
    parameter bit FULL_BYPASS = 1'b0,
    parameter int SIZE = `L1D_SIZE,
    parameter int WAY_NUM = `L1D_WAY_NUM,
    parameter int LINE_SIZE = `L1_LINE_SIZE,
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
    localparam int WORD_SIZE = 4;
    localparam int WORD_NUM = LINE_SIZE / WORD_SIZE;
    localparam int WORD_IDX_W = $clog2(WORD_NUM);
    localparam int LINE_OFF_W = $clog2(LINE_SIZE);
    localparam int SET_NUM = SIZE / (WAY_NUM * LINE_SIZE);
    localparam int SET_W = $clog2(SET_NUM);
    localparam int WAY_W = WAY_NUM <= 1 ? 1 : $clog2(WAY_NUM);
    localparam int TAG_W = 32 - LINE_OFF_W - SET_W;
    localparam int DATA_AW = SET_W + WORD_IDX_W;
    localparam logic [WORD_IDX_W-1:0] LAST_BEAT = WORD_IDX_W'(WORD_NUM - 1);
    localparam logic [WAY_W-1:0] LAST_WAY = WAY_W'(WAY_NUM - 1);
    localparam logic [SET_W-1:0] LAST_SET = SET_W'(SET_NUM - 1);
    localparam logic [7:0] AXI_LINE_LEN = 8'(WORD_NUM - 1);

    typedef enum logic [4:0] {
        S_IDLE,
        S_BYPASS_RD_SEND0,
        S_BYPASS_RD_RESP0,
        S_BYPASS_RD_SEND1,
        S_BYPASS_RD_RESP1,
        S_BYPASS_WR_SEND0,
        S_BYPASS_WR_RESP0,
        S_BYPASS_WR_SEND1,
        S_BYPASS_WR_RESP1,
        S_LOOKUP,
        S_LOOKUP_CHECK,
        S_WB_AW,
        S_WB_READ,
        S_WB_CAPTURE,
        S_WB_W,
        S_WB_B,
        S_REFILL_AR,
        S_REFILL_R,
        S_SERVE_READ,
        S_SERVE_USE,
        S_RESP,
        S_FLUSH_CHECK,
        S_FLUSH_WB_AW,
        S_FLUSH_WB_READ,
        S_FLUSH_WB_CAPTURE,
        S_FLUSH_WB_W,
        S_FLUSH_WB_B,
        S_FLUSH_DONE
    } state_t;

    state_t state;

    logic [15:0] req_trans_id;
    bti_req_cmd_t req_cmd;
    bti_req_size_t req_size;
    logic [31:0] req_addr;
    logic [31:0] req_data;
    logic [3:0] req_strobe;
    logic req_split;

    logic [2:0] req_byte_idx;
    logic [31:0] rsp_data;
    logic rsp_ok;

    logic [SET_W-1:0] req_set;
    logic [WAY_W-1:0] req_way;
    logic [TAG_W-1:0] req_tag;
    logic [31:0] req_line_addr;
    logic [31:0] wb_line_addr;
    logic [WORD_IDX_W-1:0] beat_idx;
    logic aw_done;
    logic w_done;
    logic refill_ok;
    logic [31:0] wb_word_data;
    logic [SET_W-1:0] flush_set;
    logic [WAY_W-1:0] flush_way;
    logic flush_pending;

    logic [TAG_W-1:0] tag_ram[WAY_NUM][SET_NUM];
    logic valid_ram[WAY_NUM][SET_NUM];
    logic dirty_ram[WAY_NUM][SET_NUM];
    logic [WAY_W-1:0] replace_way[SET_NUM];

    sram_rw_if_t #(DATA_AW, 32) data_rw_if[WAY_NUM]();
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

    wire aw_hsk = mem_axi4_aw_mst.vld && mem_axi4_aw_mst.rdy;
    wire w_hsk = mem_axi4_w_mst.vld && mem_axi4_w_mst.rdy;
    wire b_hsk = mem_axi4_b_slv.vld && mem_axi4_b_slv.rdy;
    wire ar_hsk = mem_axi4_ar_mst.vld && mem_axi4_ar_mst.rdy;
    wire r_hsk = mem_axi4_r_slv.vld && mem_axi4_r_slv.rdy;
    wire rsp_hsk = host_bti_rsp_mst.vld && host_bti_rsp_mst.rdy;
    wire [31:0] cur_addr = req_addr + {29'b0, req_byte_idx};
    wire [SET_W-1:0] cur_set = cur_addr[LINE_OFF_W +: SET_W];
    wire [TAG_W-1:0] cur_tag = cur_addr[31 -: TAG_W];
    wire [WORD_IDX_W-1:0] cur_word_idx = cur_addr[2 +: WORD_IDX_W];
    wire [1:0] cur_byte_off = cur_addr[1:0];
    wire [DATA_AW-1:0] cur_data_addr = {req_set, cur_word_idx};
    wire [DATA_AW-1:0] beat_data_addr = {req_set, beat_idx};
    wire [DATA_AW-1:0] flush_data_addr = {flush_set, beat_idx};
    wire [31:0] req_aligned_addr = {req_addr[31:2], 2'b00};
    wire bypass_second = state == S_BYPASS_RD_SEND1 ||
        state == S_BYPASS_RD_RESP1 ||
        state == S_BYPASS_WR_SEND1 ||
        state == S_BYPASS_WR_RESP1;
    wire flush_active =
        state == S_FLUSH_CHECK ||
        state == S_FLUSH_WB_AW ||
        state == S_FLUSH_WB_READ ||
        state == S_FLUSH_WB_CAPTURE ||
        state == S_FLUSH_WB_W ||
        state == S_FLUSH_WB_B ||
        state == S_FLUSH_DONE;
    wire flush_req = flush_slv.vld || flush_pending;

    function automatic logic [WAY_W-1:0] way_idx(input int unsigned idx);
        way_idx = idx[WAY_W-1:0];
    endfunction

    function automatic logic [SET_W-1:0] set_idx(input int unsigned idx);
        set_idx = idx[SET_W-1:0];
    endfunction

    function automatic logic [2:0] req_byte_num(
        input bti_req_size_t size
    );
        unique case (size)
        BTI_REQ_SIZE_B1: req_byte_num = 3'd1;
        BTI_REQ_SIZE_B2: req_byte_num = 3'd2;
        default: req_byte_num = 3'd4;
        endcase
    endfunction

    function automatic logic addr_in_range(
        input logic [31:0] addr,
        input logic [31:0] base,
        input logic [31:0] size
    );
        addr_in_range = size != 32'b0 && addr >= base && addr < base + size;
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
        input logic [1:0] off,
        input bti_req_size_t size
    );
        req_cross_word = ({1'b0, off} + req_byte_num(size)) > 3'd4;
    endfunction

    function automatic logic axi_resp_ok(input logic [1:0] resp);
        axi_resp_ok = resp == 2'b00;
    endfunction

    function automatic logic [31:0] first_read_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        first_read_data = data >> ({3'b0, off} << 3);
    endfunction

    function automatic logic [31:0] second_read_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        second_read_data = data << ((3'd4 - {1'b0, off}) << 3);
    endfunction

    function automatic logic [31:0] first_write_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        first_write_data = data << ({3'b0, off} << 3);
    endfunction

    function automatic logic [3:0] first_write_strobe(
        input logic [3:0] strobe,
        input logic [1:0] off
    );
        first_write_strobe = (strobe << off) & 4'hf;
    endfunction

    function automatic logic [31:0] second_write_data(
        input logic [31:0] data,
        input logic [1:0] off
    );
        second_write_data = data >> ((3'd4 - {1'b0, off}) << 3);
    endfunction

    function automatic logic [3:0] second_write_strobe(
        input logic [3:0] strobe,
        input logic [1:0] off
    );
        second_write_strobe = strobe >> (3'd4 - {1'b0, off});
    endfunction

    function automatic logic [31:0] write_byte(
        input logic [31:0] word,
        input logic [1:0] off,
        input logic [7:0] byte_val
    );
        logic [31:0] mask;
        begin
            mask = 32'hff << ({3'b0, off} << 3);
            write_byte = (word & ~mask) | ({24'b0, byte_val} << ({3'b0, off} << 3));
        end
    endfunction

    function automatic logic [7:0] read_byte(
        input logic [31:0] word,
        input logic [1:0] off
    );
        logic [31:0] shifted;
        begin
            shifted = word >> ({3'b0, off} << 3);
            read_byte = shifted[7:0];
        end
    endfunction

    genvar way;
    generate
        for (way = 0; way < WAY_NUM; way++) begin : gen_data_way
            localparam logic [WAY_W-1:0] WAY_IDX = way_idx(way);

            assign data_rw_if[way].cs = data_cs && data_way == WAY_IDX;
            assign data_rw_if[way].addr = data_addr;
            assign data_rw_if[way].wen = data_wen;
            assign data_rw_if[way].wdata = data_wdata;
            assign way_rdata[way] = data_rw_if[way].rdata;

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
            if (!hit && valid_ram[i][req_set] &&
                tag_ram[i][req_set] == req_tag) begin
                hit = 1'b1;
                hit_way = way_idx(i);
            end
        end
    end

    always_comb begin
        found_invalid = 1'b0;
        victim_way = replace_way[req_set];
        for (int unsigned i = 0; i < WAY_NUM; i++) begin
            if (!found_invalid && !valid_ram[i][req_set]) begin
                found_invalid = 1'b1;
                victim_way = way_idx(i);
            end
        end
        victim_dirty = dirty_ram[victim_way][req_set];
        victim_tag = tag_ram[victim_way][req_set];
    end

    always_comb begin
        logic [31:0] req_data_shifted;

        data_cs = 1'b0;
        data_wen = 1'b0;
        data_way = req_way;
        data_addr = beat_data_addr;
        data_wdata = 32'b0;
        req_data_shifted = req_data >> ({3'b0, req_byte_idx} << 3);

        unique case (state)
        S_WB_READ: begin
            data_cs = 1'b1;
            data_way = req_way;
            data_addr = beat_data_addr;
        end
        S_FLUSH_WB_READ: begin
            data_cs = 1'b1;
            data_way = flush_way;
            data_addr = flush_data_addr;
        end
        S_FLUSH_WB_CAPTURE: begin
            data_way = flush_way;
        end
        S_REFILL_R: begin
            data_cs = r_hsk;
            data_wen = r_hsk;
            data_way = req_way;
            data_addr = beat_data_addr;
            data_wdata = mem_axi4_r_slv.pkt.data;
        end
        S_SERVE_READ: begin
            data_cs = 1'b1;
            data_way = req_way;
            data_addr = cur_data_addr;
        end
        S_SERVE_USE: begin
            data_cs = req_cmd == BTI_REQ_CMD_WRITE &&
                req_strobe[req_byte_idx[1:0]];
            data_wen = data_cs;
            data_way = req_way;
            data_addr = cur_data_addr;
            data_wdata = write_byte(
                data_rdata,
                cur_byte_off,
                req_data_shifted[7:0]
            );
        end
        default: ;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE;
            req_trans_id <= '0;
            req_cmd <= BTI_REQ_CMD_READ;
            req_size <= BTI_REQ_SIZE_B1;
            req_addr <= '0;
            req_data <= '0;
            req_strobe <= '0;
            req_split <= 1'b0;
            req_byte_idx <= '0;
            rsp_data <= '0;
            rsp_ok <= 1'b1;
            req_set <= '0;
            req_way <= '0;
            req_tag <= '0;
            req_line_addr <= '0;
            wb_line_addr <= '0;
            beat_idx <= '0;
            aw_done <= 1'b0;
            w_done <= 1'b0;
            refill_ok <= 1'b1;
            wb_word_data <= '0;
            flush_set <= '0;
            flush_way <= '0;
            flush_pending <= 1'b0;
            for (int w = 0; w < WAY_NUM; w++) begin
                for (int s = 0; s < SET_NUM; s++) begin
                    tag_ram[w][s] <= '0;
                    valid_ram[w][s] <= 1'b0;
                    dirty_ram[w][s] <= 1'b0;
                end
            end
            for (int s = 0; s < SET_NUM; s++)
                replace_way[s] <= '0;
        end else begin
            if (flush_slv.vld && state != S_IDLE && !flush_active)
                flush_pending <= 1'b1;

            unique case (state)
            S_IDLE: begin
                if (flush_req) begin
                    flush_set <= '0;
                    flush_way <= '0;
                    flush_pending <= 1'b0;
                    state <= S_FLUSH_CHECK;
                end else if (host_bti_req_slv.vld && host_bti_req_slv.rdy) begin
                    req_trans_id <= host_bti_req_slv.pkt.trans_id;
                    req_cmd <= host_bti_req_slv.pkt.cmd;
                    req_size <= host_bti_req_slv.pkt.size;
                    req_addr <= host_bti_req_slv.pkt.addr;
                    req_data <= host_bti_req_slv.pkt.data;
                    req_strobe <= host_bti_req_slv.pkt.strobe;
                    req_split <= req_cross_word(host_bti_req_slv.pkt.addr[1:0],
                        host_bti_req_slv.pkt.size);
                    req_byte_idx <= '0;
                    rsp_data <= '0;
                    rsp_ok <= 1'b1;
                    aw_done <= 1'b0;
                    w_done <= 1'b0;
                    refill_ok <= 1'b1;
                    if (bypass_addr(host_bti_req_slv.pkt.addr)) begin
                        state <= host_bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE ?
                            S_BYPASS_WR_SEND0 : S_BYPASS_RD_SEND0;
                    end else if (RO &&
                        host_bti_req_slv.pkt.cmd == BTI_REQ_CMD_WRITE) begin
                        rsp_ok <= 1'b0;
                        state <= S_RESP;
                    end else begin
                        state <= S_LOOKUP;
                    end
                end
            end
            S_BYPASS_RD_SEND0: begin
                if (ar_hsk)
                    state <= S_BYPASS_RD_RESP0;
            end
            S_BYPASS_RD_RESP0: begin
                if (r_hsk) begin
                    rsp_data <= first_read_data(mem_axi4_r_slv.pkt.data,
                        req_addr[1:0]);
                    rsp_ok <= axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                    state <= req_split ? S_BYPASS_RD_SEND1 : S_RESP;
                end
            end
            S_BYPASS_RD_SEND1: begin
                if (ar_hsk)
                    state <= S_BYPASS_RD_RESP1;
            end
            S_BYPASS_RD_RESP1: begin
                if (r_hsk) begin
                    rsp_data <= rsp_data |
                        second_read_data(mem_axi4_r_slv.pkt.data, req_addr[1:0]);
                    rsp_ok <= rsp_ok && axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                    state <= S_RESP;
                end
            end
            S_BYPASS_WR_SEND0: begin
                if (aw_hsk)
                    aw_done <= 1'b1;
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_done || aw_hsk) && (w_done || w_hsk))
                    state <= S_BYPASS_WR_RESP0;
            end
            S_BYPASS_WR_RESP0: begin
                if (b_hsk) begin
                    rsp_ok <= axi_resp_ok(mem_axi4_b_slv.pkt.resp);
                    if (req_split) begin
                        aw_done <= 1'b0;
                        w_done <= 1'b0;
                        state <= S_BYPASS_WR_SEND1;
                    end else begin
                        state <= S_RESP;
                    end
                end
            end
            S_BYPASS_WR_SEND1: begin
                if (aw_hsk)
                    aw_done <= 1'b1;
                if (w_hsk)
                    w_done <= 1'b1;
                if ((aw_done || aw_hsk) && (w_done || w_hsk))
                    state <= S_BYPASS_WR_RESP1;
            end
            S_BYPASS_WR_RESP1: begin
                if (b_hsk) begin
                    rsp_ok <= rsp_ok && axi_resp_ok(mem_axi4_b_slv.pkt.resp);
                    state <= S_RESP;
                end
            end
            S_LOOKUP: begin
                req_set <= cur_set;
                req_tag <= cur_tag;
                req_line_addr <= {cur_addr[31:LINE_OFF_W], {LINE_OFF_W{1'b0}}};
                state <= S_LOOKUP_CHECK;
            end
            S_LOOKUP_CHECK: begin
                if (hit) begin
                    req_way <= hit_way;
                    state <= S_SERVE_READ;
                end else begin
                    req_way <= victim_way;
                    if (!found_invalid)
                        replace_way[req_set] <= victim_way + 1'b1;
                    if (valid_ram[victim_way][req_set] && victim_dirty) begin
                        wb_line_addr <= {victim_tag, req_set, {LINE_OFF_W{1'b0}}};
                        beat_idx <= '0;
                        state <= S_WB_AW;
                    end else begin
                        beat_idx <= '0;
                        state <= S_REFILL_AR;
                    end
                end
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
                    dirty_ram[req_way][req_set] <= 1'b0;
                    if (axi_resp_ok(mem_axi4_b_slv.pkt.resp)) begin
                        beat_idx <= '0;
                        state <= S_REFILL_AR;
                    end else begin
                        valid_ram[req_way][req_set] <= 1'b0;
                        rsp_ok <= 1'b0;
                        state <= S_RESP;
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
                        tag_ram[req_way][req_set] <= req_tag;
                        valid_ram[req_way][req_set] <= refill_ok &&
                            axi_resp_ok(mem_axi4_r_slv.pkt.resp);
                        dirty_ram[req_way][req_set] <= 1'b0;
                        if (refill_ok && axi_resp_ok(mem_axi4_r_slv.pkt.resp)) begin
                            state <= S_SERVE_READ;
                        end else begin
                            rsp_ok <= 1'b0;
                            state <= S_RESP;
                        end
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                    end
                end
            end
            S_SERVE_READ: begin
                state <= S_SERVE_USE;
            end
            S_SERVE_USE: begin
                if (req_cmd == BTI_REQ_CMD_READ) begin
                    rsp_data <= rsp_data |
                        ({24'b0, read_byte(data_rdata, cur_byte_off)} <<
                            ({3'b0, req_byte_idx} << 3));
                end else if (req_strobe[req_byte_idx[1:0]]) begin
                    dirty_ram[req_way][req_set] <= 1'b1;
                end

                if (req_byte_idx + 3'd1 >= req_byte_num(req_size)) begin
                    rsp_ok <= 1'b1;
                    state <= S_RESP;
                end else begin
                    req_byte_idx <= req_byte_idx + 1'b1;
                    state <= S_LOOKUP;
                end
            end
            S_RESP: begin
                if (rsp_hsk)
                    state <= S_IDLE;
            end
            S_FLUSH_CHECK: begin
                if (valid_ram[flush_way][flush_set] &&
                    dirty_ram[flush_way][flush_set]) begin
                    wb_line_addr <= {tag_ram[flush_way][flush_set],
                        flush_set, {LINE_OFF_W{1'b0}}};
                    beat_idx <= '0;
                    state <= S_FLUSH_WB_AW;
                end else begin
                    valid_ram[flush_way][flush_set] <= 1'b0;
                    dirty_ram[flush_way][flush_set] <= 1'b0;
                    if (flush_way == LAST_WAY && flush_set == LAST_SET) begin
                        state <= S_FLUSH_DONE;
                    end else begin
                        if (flush_way == LAST_WAY) begin
                            flush_way <= '0;
                            flush_set <= flush_set + 1'b1;
                        end else begin
                            flush_way <= flush_way + 1'b1;
                        end
                    end
                end
            end
            S_FLUSH_WB_AW: begin
                if (aw_hsk) begin
                    beat_idx <= '0;
                    state <= S_FLUSH_WB_READ;
                end
            end
            S_FLUSH_WB_READ: begin
                state <= S_FLUSH_WB_CAPTURE;
            end
            S_FLUSH_WB_CAPTURE: begin
                wb_word_data <= data_rdata;
                state <= S_FLUSH_WB_W;
            end
            S_FLUSH_WB_W: begin
                if (w_hsk) begin
                    if (beat_idx == LAST_BEAT) begin
                        state <= S_FLUSH_WB_B;
                    end else begin
                        beat_idx <= beat_idx + 1'b1;
                        state <= S_FLUSH_WB_READ;
                    end
                end
            end
            S_FLUSH_WB_B: begin
                if (b_hsk) begin
                    valid_ram[flush_way][flush_set] <= 1'b0;
                    dirty_ram[flush_way][flush_set] <= 1'b0;
                    if (flush_way == LAST_WAY && flush_set == LAST_SET) begin
                        state <= S_FLUSH_DONE;
                    end else begin
                        if (flush_way == LAST_WAY) begin
                            flush_way <= '0;
                            flush_set <= flush_set + 1'b1;
                        end else begin
                            flush_way <= flush_way + 1'b1;
                        end
                        state <= S_FLUSH_CHECK;
                    end
                end
            end
            S_FLUSH_DONE: begin
                state <= S_IDLE;
            end
            default: begin
                state <= S_IDLE;
            end
            endcase
        end
    end

    assign host_bti_req_slv.rdy = state == S_IDLE && !flush_req;

    assign mem_axi4_ar_mst.vld =
        state == S_BYPASS_RD_SEND0 ||
        state == S_BYPASS_RD_SEND1 ||
        state == S_REFILL_AR;
    assign mem_axi4_ar_mst.pkt.id = '0;
    assign mem_axi4_ar_mst.pkt.addr =
        state == S_REFILL_AR ? req_line_addr :
        (bypass_second ? req_aligned_addr + 32'd4 : req_aligned_addr);
    assign mem_axi4_ar_mst.pkt.len = state == S_REFILL_AR ? AXI_LINE_LEN : 8'b0;
    assign mem_axi4_ar_mst.pkt.size = AXI4_AR_SIZE_B4;
    assign mem_axi4_ar_mst.pkt.burst = AXI4_AR_BURST_INCR;
    assign mem_axi4_ar_mst.pkt.lock = 1'b0;
    assign mem_axi4_ar_mst.pkt.cache = state == S_REFILL_AR ? 4'hf : 4'h0;
    assign mem_axi4_ar_mst.pkt.prot = 3'h0;
    assign mem_axi4_ar_mst.pkt.qos = 4'h0;
    assign mem_axi4_ar_mst.pkt.user = '0;

    assign mem_axi4_aw_mst.vld =
        ((state == S_BYPASS_WR_SEND0 || state == S_BYPASS_WR_SEND1) &&
            !aw_done) ||
        state == S_WB_AW ||
        state == S_FLUSH_WB_AW;
    assign mem_axi4_aw_mst.pkt.id = '0;
    assign mem_axi4_aw_mst.pkt.addr =
        (state == S_WB_AW || state == S_FLUSH_WB_AW) ? wb_line_addr :
        (bypass_second ? req_aligned_addr + 32'd4 : req_aligned_addr);
    assign mem_axi4_aw_mst.pkt.len =
        (state == S_WB_AW || state == S_FLUSH_WB_AW) ? AXI_LINE_LEN : 8'b0;
    assign mem_axi4_aw_mst.pkt.size = AXI4_AW_SIZE_B4;
    assign mem_axi4_aw_mst.pkt.burst = AXI4_AW_BURST_INCR;
    assign mem_axi4_aw_mst.pkt.lock = 1'b0;
    assign mem_axi4_aw_mst.pkt.cache =
        (state == S_WB_AW || state == S_FLUSH_WB_AW) ? 4'hf : 4'h0;
    assign mem_axi4_aw_mst.pkt.prot = 3'h0;
    assign mem_axi4_aw_mst.pkt.qos = 4'h0;
    assign mem_axi4_aw_mst.pkt.user = '0;

    assign mem_axi4_w_mst.vld =
        ((state == S_BYPASS_WR_SEND0 || state == S_BYPASS_WR_SEND1) &&
            !w_done) ||
        state == S_WB_W ||
        state == S_FLUSH_WB_W;
    assign mem_axi4_w_mst.pkt.data =
        (state == S_WB_W || state == S_FLUSH_WB_W) ? wb_word_data :
        (bypass_second ? second_write_data(req_data, req_addr[1:0]) :
            first_write_data(req_data, req_addr[1:0]));
    assign mem_axi4_w_mst.pkt.strb =
        (state == S_WB_W || state == S_FLUSH_WB_W) ? 4'hf :
        (bypass_second ? second_write_strobe(req_strobe, req_addr[1:0]) :
            first_write_strobe(req_strobe, req_addr[1:0]));
    assign mem_axi4_w_mst.pkt.last =
        (state != S_WB_W && state != S_FLUSH_WB_W) || beat_idx == LAST_BEAT;

    assign mem_axi4_r_slv.rdy =
        state == S_BYPASS_RD_RESP0 ||
        state == S_BYPASS_RD_RESP1 ||
        state == S_REFILL_R;
    assign mem_axi4_b_slv.rdy =
        state == S_BYPASS_WR_RESP0 ||
        state == S_BYPASS_WR_RESP1 ||
        state == S_WB_B ||
        state == S_FLUSH_WB_B;

    assign host_bti_rsp_mst.vld = state == S_RESP;
    assign host_bti_rsp_mst.pkt.trans_id = req_trans_id;
    assign host_bti_rsp_mst.pkt.data =
        req_cmd == BTI_REQ_CMD_WRITE ? 32'b0 : rsp_data;
    assign host_bti_rsp_mst.pkt.ok = rsp_ok;
    assign flush_ack_mst.vld = state == S_FLUSH_DONE;

    assign perf_mst.pkt.hit = state == S_LOOKUP_CHECK && hit;
    assign perf_mst.pkt.miss = state == S_LOOKUP_CHECK && !hit;
    assign perf_mst.pkt.bypass = state == S_IDLE && host_bti_req_slv.vld &&
        host_bti_req_slv.rdy && bypass_addr(host_bti_req_slv.pkt.addr);
    assign perf_mst.pkt.writeback = (state == S_WB_AW ||
        state == S_FLUSH_WB_AW) && aw_hsk;
    assign perf_mst.pkt.busy = host_bti_req_slv.vld && !host_bti_req_slv.rdy;

`ifndef SYNTHESIS
    logic rtl_progress_en;
    longint unsigned rtl_progress_cycle;

    initial begin
        rtl_progress_en = $test$plusargs("rtl_progress");
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rtl_progress_cycle <= 0;
        end else if (rtl_progress_en) begin
            rtl_progress_cycle <= rtl_progress_cycle + 1;
            if (rtl_progress_cycle[19:0] == 20'h0) begin
                $display("[RTL_PROGRESS][%m] cycle=%0d state=%0d req=%08x cmd=%0d size=%0d set=%0d way=%0d beat=%0d flush=%0d/%0d host=%0b/%0b:%0b/%0b axi_aw=%0b/%0b axi_w=%0b/%0b axi_b=%0b/%0b axi_ar=%0b/%0b axi_r=%0b/%0b rlast=%0b",
                    rtl_progress_cycle,
                    state,
                    req_addr,
                    req_cmd,
                    req_size,
                    req_set,
                    req_way,
                    beat_idx,
                    flush_set,
                    flush_way,
                    host_bti_req_slv.vld, host_bti_req_slv.rdy,
                    host_bti_rsp_mst.vld, host_bti_rsp_mst.rdy,
                    mem_axi4_aw_mst.vld, mem_axi4_aw_mst.rdy,
                    mem_axi4_w_mst.vld, mem_axi4_w_mst.rdy,
                    mem_axi4_b_slv.vld, mem_axi4_b_slv.rdy,
                    mem_axi4_ar_mst.vld, mem_axi4_ar_mst.rdy,
                    mem_axi4_r_slv.vld, mem_axi4_r_slv.rdy,
                    mem_axi4_r_slv.pkt.last);
            end
        end
    end
`endif
endmodule
