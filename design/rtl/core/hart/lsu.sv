`include "itf/ldst_req_if.svh"

module lsu(
    input logic        clk,
    input logic        rst_n,
    ldst_req_if_t.slv  exu_ldst_req_slv,
    ldst_rsp_if_t.mst  exu_ldst_rsp_mst,
    ldst_req_if_t.mst  hbi_ldst_req_mst,
    ldst_rsp_if_t.slv  hbi_ldst_rsp_slv,
    csr_lsu_state_if_t.slv csr_lsu_state_slv,
    perf_lsu_if_t.mst perf_mst
);
    localparam logic [12:0] PAGE_SIZE = 13'd4096;

    typedef enum logic [2:0] {
        S_IDLE,
        S_DIRECT_REQ,
        S_DIRECT_RSP,
        S_SPLIT_REQ,
        S_SPLIT_RSP,
        S_DONE
    } state_t;

    state_t state;
    state_t state_nxt;
    logic [31:0] req_addr;
    logic req_st;
    ldst_req_size_t req_size;
    logic [31:0] req_data;
    logic [3:0] req_strobe;
    logic split_req;
    logic [2:0] byte_num;
    logic [2:0] req_byte_idx;
    logic [2:0] rsp_byte_idx;
    logic [31:0] rsp_data;
    logic rsp_ok;
    logic capture_req;
    logic capture_direct_rsp;
    logic capture_split_rsp;

    wire exu_req_hsk = exu_ldst_req_slv.vld && exu_ldst_req_slv.rdy;
    wire exu_rsp_hsk = exu_ldst_rsp_mst.vld && exu_ldst_rsp_mst.rdy;
    wire hbi_req_hsk = hbi_ldst_req_mst.vld && hbi_ldst_req_mst.rdy;
    wire hbi_rsp_hsk = hbi_ldst_rsp_slv.vld && hbi_ldst_rsp_slv.rdy;
    wire translation_en = csr_lsu_state_slv.pkt.satp[31];

    function automatic logic [2:0] req_byte_num(
        input ldst_req_size_t size
    );
        unique case (size)
        LDST_REQ_SIZE_B1: req_byte_num = 3'd1;
        LDST_REQ_SIZE_B2: req_byte_num = 3'd2;
        default: req_byte_num = 3'd4;
        endcase
    endfunction

    function automatic logic req_cross_page(
        input logic [31:0] addr,
        input ldst_req_size_t size
    );
        req_cross_page = ({1'b0, addr[11:0]} +
            {10'b0, req_byte_num(size)}) >
            PAGE_SIZE;
    endfunction

    function automatic logic [7:0] req_store_byte(
        input logic [31:0] data,
        input logic [2:0] idx
    );
        unique case (idx)
        3'd0: req_store_byte = data[7:0];
        3'd1: req_store_byte = data[15:8];
        3'd2: req_store_byte = data[23:16];
        3'd3: req_store_byte = data[31:24];
        default: req_store_byte = 8'h00;
        endcase
    endfunction

    always_comb begin
        state_nxt = state;
        capture_req = 1'b0;
        capture_direct_rsp = 1'b0;
        capture_split_rsp = 1'b0;

        unique case (state)
        S_IDLE: begin
            if (exu_req_hsk) begin
                capture_req = 1'b1;
                if (translation_en &&
                    req_cross_page(exu_ldst_req_slv.pkt.addr,
                    exu_ldst_req_slv.pkt.size))
                    state_nxt = S_SPLIT_REQ;
                else
                    state_nxt = S_DIRECT_REQ;
            end
        end
        S_DIRECT_REQ: begin
            if (hbi_req_hsk)
                state_nxt = S_DIRECT_RSP;
        end
        S_DIRECT_RSP: begin
            if (hbi_rsp_hsk) begin
                capture_direct_rsp = 1'b1;
                state_nxt = S_DONE;
            end
        end
        S_SPLIT_REQ: begin
            if (hbi_req_hsk)
                state_nxt = S_SPLIT_RSP;
        end
        S_SPLIT_RSP: begin
            if (hbi_rsp_hsk) begin
                capture_split_rsp = 1'b1;
                if (!hbi_ldst_rsp_slv.pkt.ok ||
                    rsp_byte_idx + 3'd1 == byte_num)
                    state_nxt = S_DONE;
                else
                    state_nxt = S_SPLIT_REQ;
            end
        end
        S_DONE: begin
            if (exu_rsp_hsk)
                state_nxt = S_IDLE;
        end
        default: begin
            state_nxt = S_IDLE;
        end
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S_IDLE;
            req_addr <= '0;
            req_st <= 1'b0;
            req_size <= LDST_REQ_SIZE_B1;
            req_data <= '0;
            req_strobe <= '0;
            split_req <= 1'b0;
            byte_num <= '0;
            req_byte_idx <= '0;
            rsp_byte_idx <= '0;
            rsp_data <= '0;
            rsp_ok <= 1'b1;
        end else begin
            state <= state_nxt;

            if (capture_req) begin
                req_addr <= exu_ldst_req_slv.pkt.addr;
                req_st <= exu_ldst_req_slv.pkt.st;
                req_size <= exu_ldst_req_slv.pkt.size;
                req_data <= exu_ldst_req_slv.pkt.data;
                req_strobe <= exu_ldst_req_slv.pkt.strobe;
                split_req <= translation_en &&
                    req_cross_page(exu_ldst_req_slv.pkt.addr,
                    exu_ldst_req_slv.pkt.size);
                byte_num <= req_byte_num(exu_ldst_req_slv.pkt.size);
                req_byte_idx <= '0;
                rsp_byte_idx <= '0;
                rsp_data <= '0;
                rsp_ok <= 1'b1;
            end else begin
                if (hbi_req_hsk && state == S_SPLIT_REQ)
                    req_byte_idx <= req_byte_idx + 3'd1;

                if (capture_direct_rsp) begin
                    rsp_data <= hbi_ldst_rsp_slv.pkt.data;
                    rsp_ok <= hbi_ldst_rsp_slv.pkt.ok;
                end

                if (capture_split_rsp) begin
                    rsp_ok <= rsp_ok && hbi_ldst_rsp_slv.pkt.ok;
                    if (!req_st && hbi_ldst_rsp_slv.pkt.ok)
                        rsp_data[rsp_byte_idx * 8 +: 8] <=
                            hbi_ldst_rsp_slv.pkt.data[7:0];
                    rsp_byte_idx <= rsp_byte_idx + 3'd1;
                end
            end
        end
    end

    assign exu_ldst_req_slv.rdy = state == S_IDLE;

    assign hbi_ldst_req_mst.vld = state == S_DIRECT_REQ ||
        state == S_SPLIT_REQ;
    assign hbi_ldst_req_mst.pkt.addr = split_req ?
        (req_addr + {29'b0, req_byte_idx}) : req_addr;
    assign hbi_ldst_req_mst.pkt.st = req_st;
    assign hbi_ldst_req_mst.pkt.size = split_req ?
        LDST_REQ_SIZE_B1 : req_size;
    assign hbi_ldst_req_mst.pkt.data = split_req ?
        {24'b0, req_store_byte(req_data, req_byte_idx)} : req_data;
    assign hbi_ldst_req_mst.pkt.strobe = split_req ?
        (req_st ? 4'b0001 : 4'b0000) : req_strobe;

    assign hbi_ldst_rsp_slv.rdy = state == S_DIRECT_RSP ||
        state == S_SPLIT_RSP;

    assign exu_ldst_rsp_mst.vld = state == S_DONE;
    assign exu_ldst_rsp_mst.pkt.data = req_st ? 32'b0 : rsp_data;
    assign exu_ldst_rsp_mst.pkt.ok = rsp_ok;

    assign perf_mst.pkt.exu_req = exu_req_hsk;
    assign perf_mst.pkt.direct_req_bp = state == S_DIRECT_REQ &&
        hbi_ldst_req_mst.vld && !hbi_ldst_req_mst.rdy;
    assign perf_mst.pkt.direct_rsp_wait = state == S_DIRECT_RSP &&
        !hbi_ldst_rsp_slv.vld;
    assign perf_mst.pkt.split_req = exu_req_hsk && translation_en &&
        req_cross_page(exu_ldst_req_slv.pkt.addr, exu_ldst_req_slv.pkt.size);
    assign perf_mst.pkt.split_req_bp = state == S_SPLIT_REQ &&
        hbi_ldst_req_mst.vld && !hbi_ldst_req_mst.rdy;
    assign perf_mst.pkt.split_rsp_wait = state == S_SPLIT_RSP &&
        !hbi_ldst_rsp_slv.vld;
    assign perf_mst.pkt.exu_rsp_bp = exu_ldst_rsp_mst.vld &&
        !exu_ldst_rsp_mst.rdy;
endmodule
