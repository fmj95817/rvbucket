module tlb #(
    parameter ENTRY_NUM = 32
)(
    input logic        clk,
    input logic        rst_n,
    input logic        flush,
    input logic        lookup_vld,
    input logic [31:0] lookup_va,
    input logic [21:0] lookup_satp_ppn,
    input logic [8:0]  lookup_satp_asid,
    output logic       lookup_rsp_vld,
    output logic       lookup_hit,
    output logic [31:0] lookup_pte,
    output logic       lookup_level,
    input logic        fill_vld,
    input logic [31:0] fill_va,
    input logic [21:0] fill_satp_ppn,
    input logic [8:0]  fill_satp_asid,
    input logic        fill_level,
    input logic [31:0] fill_pte
);
    localparam WAY_NUM = 4;
    localparam SET_NUM = ENTRY_NUM / WAY_NUM;
    localparam SET_W = $clog2(SET_NUM);
    localparam WAY_W = $clog2(WAY_NUM);
    localparam META_W = 53;
    localparam logic [SET_W-1:0] LAST_SET = SET_W'(SET_NUM - 1);

    typedef struct packed {
        logic        valid;
        logic        level;
        logic [8:0]  satp_asid;
        logic [21:0] satp_ppn;
        logic [19:0] tag;
    } tlb_meta_t;

    sram_rw_if_t #(SET_W, 32) tlb_r_if[WAY_NUM]();
    sram_rw_if_t #(SET_W, 32) tlb_w_if[WAY_NUM]();
    sram_rw_if_t #(SET_W, META_W) meta_r_if[WAY_NUM]();
    sram_rw_if_t #(SET_W, META_W) meta_w_if[WAY_NUM]();

    logic [SET_W-1:0] lookup_set;
    logic [SET_W-1:0] fill_set;
    logic [SET_W-1:0] rsp_set;
    logic [SET_W-1:0] init_set;
    logic [19:0] rsp_tag;
    logic [21:0] rsp_satp_ppn;
    logic [8:0] rsp_satp_asid;
    logic init_active;

    logic [WAY_W-1:0] replace_way[SET_NUM];
    logic [WAY_W-1:0] fill_way;
    logic [WAY_NUM-1:0] way_hit;
    logic [31:0] way_rdata[WAY_NUM];
    tlb_meta_t way_meta[WAY_NUM];
    tlb_meta_t fill_meta;

    assign lookup_set = lookup_va[22 +: SET_W];
    assign fill_set = fill_va[22 +: SET_W];
    assign fill_way = replace_way[fill_set];
    assign fill_meta = '{
        valid: 1'b1,
        level: fill_level,
        satp_asid: fill_satp_asid,
        satp_ppn: fill_satp_ppn,
        tag: fill_va[31:12]
    };

    genvar way;
    generate
        for (way = 0; way < WAY_NUM; way++) begin : gen_way
            localparam logic [WAY_W-1:0] WAY_IDX = WAY_W'(way);

            assign tlb_r_if[way].cs = lookup_vld;
            assign tlb_r_if[way].addr = lookup_set;
            assign tlb_r_if[way].wen = 1'b0;
            assign tlb_r_if[way].wdata = 32'b0;

            assign tlb_w_if[way].cs = fill_vld && fill_way == WAY_IDX;
            assign tlb_w_if[way].addr = fill_set;
            assign tlb_w_if[way].wen = fill_vld && fill_way == WAY_IDX;
            assign tlb_w_if[way].wdata = fill_pte;

            assign meta_r_if[way].cs = lookup_vld;
            assign meta_r_if[way].addr = lookup_set;
            assign meta_r_if[way].wen = 1'b0;
            assign meta_r_if[way].wdata = '0;

            assign meta_w_if[way].cs = init_active ||
                (fill_vld && fill_way == WAY_IDX);
            assign meta_w_if[way].addr = init_active ? init_set : fill_set;
            assign meta_w_if[way].wen = init_active ||
                (fill_vld && fill_way == WAY_IDX);
            assign meta_w_if[way].wdata = init_active ? '0 : fill_meta;

            dp_sram #(
                .AW          (SET_W),
                .DW          (32)
            ) u_tlb_sram(
                .clk         (clk),
                .sram_r_slv  (tlb_r_if[way]),
                .sram_w_slv  (tlb_w_if[way])
            );

            dp_sram #(
                .AW          (SET_W),
                .DW          (META_W)
            ) u_meta_sram(
                .clk         (clk),
                .sram_r_slv  (meta_r_if[way]),
                .sram_w_slv  (meta_w_if[way])
            );

            assign way_rdata[way] = tlb_r_if[way].rdata;
            assign way_meta[way] = meta_r_if[way].rdata;

            assign way_hit[way] = lookup_rsp_vld && !init_active &&
                way_meta[way].valid &&
                (way_meta[way].level ?
                    way_meta[way].tag[19:10] == rsp_tag[19:10] :
                    way_meta[way].tag == rsp_tag) &&
                way_meta[way].satp_ppn == rsp_satp_ppn &&
                way_meta[way].satp_asid == rsp_satp_asid;
        end
    endgenerate

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            lookup_rsp_vld <= 1'b0;
            init_active <= 1'b1;
            init_set <= '0;
        end else begin
            if (flush) begin
                init_active <= 1'b1;
                init_set <= '0;
            end else if (init_active) begin
                replace_way[init_set] <= '0;
                if (init_set == LAST_SET)
                    init_active <= 1'b0;
                else
                    init_set <= init_set + 1'b1;
            end else if (fill_vld) begin
                replace_way[fill_set] <= fill_way + 1'b1;
            end

            lookup_rsp_vld <= lookup_vld;
            rsp_set <= lookup_set;
            rsp_tag <= lookup_va[31:12];
            rsp_satp_ppn <= lookup_satp_ppn;
            rsp_satp_asid <= lookup_satp_asid;
        end
    end

    always_comb begin
        lookup_pte = 32'b0;
        lookup_level = 1'b0;
        for (int i = 0; i < WAY_NUM; i++) begin
            if (way_hit[i]) begin
                lookup_pte = way_rdata[i];
                lookup_level = way_meta[i].level;
            end
        end
    end

    assign lookup_hit = |way_hit;
endmodule
