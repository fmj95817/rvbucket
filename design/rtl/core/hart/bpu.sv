`include "spec/core/isa.svh"
`include "spec/core/hart.svh"

module bpu(
    input logic clk,
    input logic rst_n,
    bpu_pred_req_if_t.slv pred_req_slv,
    bpu_pred_rsp_if_t.mst pred_rsp_mst,
    bpu_update_if_t.slv update_slv
);
    localparam int COND_BHT_SIZE = `BPU_COND_BHT_SIZE;
    localparam int COND_BHT_AW = $clog2(COND_BHT_SIZE);
    localparam int JALR_BTB_SIZE = `BPU_JALR_BTB_SIZE;
    localparam int JALR_BTB_WAY_NUM = `BPU_JALR_BTB_WAY_NUM;
    localparam int JALR_BTB_SET_NUM = JALR_BTB_SIZE / JALR_BTB_WAY_NUM;
    localparam int JALR_BTB_SET_AW = $clog2(JALR_BTB_SET_NUM);
    localparam int JALR_BTB_WAY_AW = $clog2(JALR_BTB_WAY_NUM);
    localparam int JALR_BTB_TAG_W = `RV_PC_SIZE - JALR_BTB_SET_AW - 2;
    localparam int RAS_SIZE = `BPU_RAS_SIZE;
    localparam int RAS_AW = $clog2(RAS_SIZE);

    localparam logic [`RV_GPR_AW-1:0] RAS_LINK_RA = 5'd1;
    localparam logic [`RV_GPR_AW-1:0] RAS_LINK_T0 = 5'd5;

    logic cond_bht_vld[COND_BHT_SIZE];
    logic [1:0] cond_bht_counter[COND_BHT_SIZE];

    logic jalr_btb_vld[JALR_BTB_WAY_NUM][JALR_BTB_SET_NUM];
    logic [JALR_BTB_TAG_W-1:0] jalr_btb_tag[JALR_BTB_WAY_NUM][JALR_BTB_SET_NUM];
    logic [`RV_PC_SIZE-1:0] jalr_btb_target_pc[JALR_BTB_WAY_NUM][JALR_BTB_SET_NUM];
    logic [JALR_BTB_WAY_AW-1:0] jalr_btb_replace_way[JALR_BTB_SET_NUM];

    logic [`RV_PC_SIZE-1:0] ras_entry[RAS_SIZE];
    logic [RAS_AW-1:0] ras_sp;
    logic [RAS_AW:0] ras_count;

    function automatic logic is_link_reg(input logic [`RV_GPR_AW-1:0] reg_idx);
        is_link_reg = reg_idx == RAS_LINK_RA || reg_idx == RAS_LINK_T0;
    endfunction

    function automatic logic jalr_reads_ras(input rv32g_inst_t inst);
        logic rd_link;
        logic rs1_link;
        begin
            rd_link = is_link_reg(inst.i.rd);
            rs1_link = is_link_reg(inst.i.rs1);
            jalr_reads_ras = rs1_link && (!rd_link || inst.i.rd != inst.i.rs1);
        end
    endfunction

    function automatic logic [COND_BHT_AW-1:0] cond_bht_idx(
        input logic [`RV_PC_SIZE-1:0] pc
    );
        cond_bht_idx = pc[COND_BHT_AW+1:2];
    endfunction

    function automatic logic [JALR_BTB_SET_AW-1:0] jalr_btb_set(
        input logic [`RV_PC_SIZE-1:0] pc
    );
        jalr_btb_set = pc[JALR_BTB_SET_AW+1:2];
    endfunction

    function automatic logic [JALR_BTB_TAG_W-1:0] jalr_btb_tag_decode(
        input logic [`RV_PC_SIZE-1:0] pc
    );
        jalr_btb_tag_decode = pc[`RV_PC_SIZE-1:JALR_BTB_SET_AW+2];
    endfunction

    function automatic logic [RAS_AW-1:0] ras_inc(
        input logic [RAS_AW-1:0] ptr
    );
        ras_inc = ptr == RAS_AW'(RAS_SIZE - 1) ? {RAS_AW{1'b0}} :
            ptr + RAS_AW'(1);
    endfunction

    function automatic logic [RAS_AW-1:0] ras_dec(
        input logic [RAS_AW-1:0] ptr
    );
        ras_dec = ptr == '0 ? RAS_AW'(RAS_SIZE - 1) : ptr - RAS_AW'(1);
    endfunction

    function automatic logic [1:0] sat_counter_update(
        input logic [1:0] counter,
        input logic taken
    );
        if (taken)
            sat_counter_update = counter == 2'd3 ? 2'd3 : counter + 2'd1;
        else
            sat_counter_update = counter == 2'd0 ? 2'd0 : counter - 2'd1;
    endfunction

    function automatic logic [`RV_XLEN-1:0] j_imm_decode(
        input rv32g_inst_t inst
    );
        j_imm_decode = {
            {(`RV_XLEN-21){inst.j.imm_20}},
            inst.j.imm_20,
            inst.j.imm_19_12,
            inst.j.imm_11,
            inst.j.imm_10_1,
            1'b0
        };
    endfunction

    function automatic logic [`RV_XLEN-1:0] b_imm_decode(
        input rv32g_inst_t inst
    );
        b_imm_decode = {
            {(`RV_XLEN-13){inst.b.imm_12}},
            inst.b.imm_12,
            inst.b.imm_11,
            inst.b.imm_10_5,
            inst.b.imm_4_1,
            1'b0
        };
    endfunction

    rv32g_inst_t pred_inst;
    rv32g_inst_t update_inst;

    logic [`RV_XLEN-1:0] pred_b_imm;
    logic signed [`RV_XLEN-1:0] pred_b_imm_s;
    logic [COND_BHT_AW-1:0] pred_bht_idx;
    logic [COND_BHT_AW-1:0] update_bht_idx;
    logic pred_bht_vld;
    logic [1:0] pred_bht_counter;

    logic [JALR_BTB_SET_AW-1:0] pred_btb_set;
    logic [JALR_BTB_SET_AW-1:0] update_btb_set;
    logic [JALR_BTB_TAG_W-1:0] pred_btb_tag;
    logic [JALR_BTB_TAG_W-1:0] update_btb_tag;
    logic pred_btb_hit;
    logic [`RV_PC_SIZE-1:0] pred_btb_target_pc;
    logic update_btb_hit;
    logic update_btb_has_invalid;
    logic [JALR_BTB_WAY_AW-1:0] update_btb_hit_way;
    logic [JALR_BTB_WAY_AW-1:0] update_btb_invalid_way;
    logic [JALR_BTB_WAY_AW-1:0] update_btb_alloc_way;

    logic pred_ras_hit;
    logic [`RV_PC_SIZE-1:0] pred_ras_target_pc;
    logic [RAS_AW-1:0] ras_eff_sp;
    logic [RAS_AW:0] ras_eff_count;
    logic ras_eff_push;
    logic [`RV_PC_SIZE-1:0] ras_eff_push_pc;

    assign pred_inst.raw = pred_req_slv.pkt.ir;
    assign update_inst.raw = update_slv.pkt.ir;
    assign pred_b_imm = b_imm_decode(pred_inst);
    assign pred_b_imm_s = pred_b_imm;
    assign pred_bht_idx = cond_bht_idx(pred_req_slv.pkt.pc);
    assign update_bht_idx = cond_bht_idx(update_slv.pkt.pc);
    assign pred_btb_set = jalr_btb_set(pred_req_slv.pkt.pc);
    assign update_btb_set = jalr_btb_set(update_slv.pkt.pc);
    assign pred_btb_tag = jalr_btb_tag_decode(pred_req_slv.pkt.pc);
    assign update_btb_tag = jalr_btb_tag_decode(update_slv.pkt.pc);

    always_comb begin
        pred_bht_vld = cond_bht_vld[pred_bht_idx];
        pred_bht_counter = cond_bht_counter[pred_bht_idx];

        if (update_slv.pkt.vld &&
            update_inst.base.opcode == OPCODE_BRANCH &&
            update_bht_idx == pred_bht_idx) begin
            pred_bht_vld = 1'b1;
            if (!cond_bht_vld[pred_bht_idx])
                pred_bht_counter = update_slv.pkt.taken ? 2'd2 : 2'd1;
            else
                pred_bht_counter = sat_counter_update(
                    cond_bht_counter[pred_bht_idx],
                    update_slv.pkt.taken
                );
        end
    end

    always_comb begin
        pred_btb_hit = 1'b0;
        pred_btb_target_pc = {`RV_PC_SIZE{1'b0}};

        if (update_slv.pkt.vld &&
            update_inst.base.opcode == OPCODE_JALR &&
            update_btb_set == pred_btb_set &&
            update_btb_tag == pred_btb_tag) begin
            pred_btb_hit = 1'b1;
            pred_btb_target_pc = update_slv.pkt.target_pc;
        end else begin
            for (int way = 0; way < JALR_BTB_WAY_NUM; way++) begin
                if (jalr_btb_vld[way][pred_btb_set] &&
                    jalr_btb_tag[way][pred_btb_set] == pred_btb_tag) begin
                    pred_btb_hit = 1'b1;
                    pred_btb_target_pc = jalr_btb_target_pc[way][pred_btb_set];
                end
            end
        end
    end

    always_comb begin
        update_btb_hit = 1'b0;
        update_btb_has_invalid = 1'b0;
        update_btb_hit_way = '0;
        update_btb_invalid_way = '0;

        for (int way = 0; way < JALR_BTB_WAY_NUM; way++) begin
            if (jalr_btb_vld[way][update_btb_set] &&
                jalr_btb_tag[way][update_btb_set] == update_btb_tag) begin
                update_btb_hit = 1'b1;
                update_btb_hit_way = JALR_BTB_WAY_AW'(way);
            end
            if (!jalr_btb_vld[way][update_btb_set] &&
                !update_btb_has_invalid) begin
                update_btb_has_invalid = 1'b1;
                update_btb_invalid_way = JALR_BTB_WAY_AW'(way);
            end
        end

        if (update_btb_hit)
            update_btb_alloc_way = update_btb_hit_way;
        else if (update_btb_has_invalid)
            update_btb_alloc_way = update_btb_invalid_way;
        else
            update_btb_alloc_way = jalr_btb_replace_way[update_btb_set];
    end

    always_comb begin
        ras_eff_sp = ras_sp;
        ras_eff_count = ras_count;
        ras_eff_push = 1'b0;
        ras_eff_push_pc = update_slv.pkt.pc + 32'd4;

        if (update_slv.pkt.vld && update_inst.base.opcode == OPCODE_JAL) begin
            if (is_link_reg(update_inst.j.rd)) begin
                ras_eff_push = 1'b1;
                ras_eff_count = ras_count < (RAS_AW + 1)'(RAS_SIZE) ?
                    ras_count + (RAS_AW + 1)'(1) : ras_count;
                ras_eff_sp = ras_inc(ras_sp);
            end
        end else if (update_slv.pkt.vld &&
            update_inst.base.opcode == OPCODE_JALR) begin
            if (!is_link_reg(update_inst.i.rd) &&
                is_link_reg(update_inst.i.rs1)) begin
                if (ras_count != '0) begin
                    ras_eff_sp = ras_dec(ras_sp);
                    ras_eff_count = ras_count - (RAS_AW + 1)'(1);
                end
            end else if (is_link_reg(update_inst.i.rd) &&
                !is_link_reg(update_inst.i.rs1)) begin
                ras_eff_push = 1'b1;
                ras_eff_count = ras_count < (RAS_AW + 1)'(RAS_SIZE) ?
                    ras_count + (RAS_AW + 1)'(1) : ras_count;
                ras_eff_sp = ras_inc(ras_sp);
            end else if (is_link_reg(update_inst.i.rd) &&
                is_link_reg(update_inst.i.rs1)) begin
                ras_eff_push = 1'b1;
                if (update_inst.i.rd != update_inst.i.rs1 &&
                    ras_count != '0) begin
                    ras_eff_count = ras_count;
                    ras_eff_sp = ras_sp;
                end else begin
                    ras_eff_count = ras_count < (RAS_AW + 1)'(RAS_SIZE) ?
                        ras_count + (RAS_AW + 1)'(1) : ras_count;
                    ras_eff_sp = ras_inc(ras_sp);
                end
            end
        end
    end

    always_comb begin
        pred_ras_hit = 1'b0;
        pred_ras_target_pc = {`RV_PC_SIZE{1'b0}};

        if (ras_eff_push) begin
            pred_ras_hit = 1'b1;
            pred_ras_target_pc = ras_eff_push_pc;
        end else if (ras_eff_count != '0) begin
            pred_ras_hit = 1'b1;
            pred_ras_target_pc = ras_entry[ras_dec(ras_eff_sp)];
        end
    end

    always_comb begin
        pred_rsp_mst.pkt = '0;
        pred_rsp_mst.pkt.vld = pred_req_slv.pkt.vld;
        pred_rsp_mst.pkt.pred_pc = pred_req_slv.pkt.pc + 32'd4;

        if (pred_req_slv.pkt.vld) begin
            pred_rsp_mst.pkt.is_ctrl =
                pred_inst.base.opcode == OPCODE_JAL ||
                pred_inst.base.opcode == OPCODE_JALR ||
                pred_inst.base.opcode == OPCODE_BRANCH;

            if (pred_inst.base.opcode == OPCODE_JAL) begin
                pred_rsp_mst.pkt.pred_taken = 1'b1;
                pred_rsp_mst.pkt.pred_pc =
                    pred_req_slv.pkt.pc + j_imm_decode(pred_inst);
            end else if (pred_inst.base.opcode == OPCODE_BRANCH) begin
                if (pred_bht_vld) begin
                    pred_rsp_mst.pkt.cond_bht_hit = 1'b1;
                    pred_rsp_mst.pkt.pred_taken = pred_bht_counter >= 2'd2;
                    if (pred_rsp_mst.pkt.pred_taken)
                        pred_rsp_mst.pkt.pred_pc =
                            pred_req_slv.pkt.pc + pred_b_imm;
                end else if (pred_b_imm_s < 0) begin
                    pred_rsp_mst.pkt.pred_taken = 1'b1;
                    pred_rsp_mst.pkt.pred_pc = pred_req_slv.pkt.pc + pred_b_imm;
                end
            end else if (pred_inst.base.opcode == OPCODE_JALR) begin
                if (jalr_reads_ras(pred_inst) && pred_ras_hit) begin
                    pred_rsp_mst.pkt.pred_taken = 1'b1;
                    pred_rsp_mst.pkt.pred_pc = pred_ras_target_pc;
                    pred_rsp_mst.pkt.jalr_ras_hit = 1'b1;
                end else if (pred_btb_hit) begin
                    pred_rsp_mst.pkt.pred_taken = 1'b1;
                    pred_rsp_mst.pkt.pred_pc = pred_btb_target_pc;
                    pred_rsp_mst.pkt.jalr_btb_hit = 1'b1;
                end else begin
                    pred_rsp_mst.pkt.jalr_btb_miss = 1'b1;
                end
            end
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ras_sp <= '0;
            ras_count <= '0;
            for (int i = 0; i < COND_BHT_SIZE; i++) begin
                cond_bht_vld[i] <= 1'b0;
                cond_bht_counter[i] <= 2'd0;
            end
            for (int set = 0; set < JALR_BTB_SET_NUM; set++) begin
                jalr_btb_replace_way[set] <= '0;
                for (int way = 0; way < JALR_BTB_WAY_NUM; way++) begin
                    jalr_btb_vld[way][set] <= 1'b0;
                    jalr_btb_tag[way][set] <= '0;
                    jalr_btb_target_pc[way][set] <= {`RV_PC_SIZE{1'b0}};
                end
            end
            for (int i = 0; i < RAS_SIZE; i++)
                ras_entry[i] <= {`RV_PC_SIZE{1'b0}};
        end else begin
            if (update_slv.pkt.vld &&
                update_inst.base.opcode == OPCODE_BRANCH) begin
                cond_bht_vld[update_bht_idx] <= 1'b1;
                if (!cond_bht_vld[update_bht_idx])
                    cond_bht_counter[update_bht_idx] <=
                        update_slv.pkt.taken ? 2'd2 : 2'd1;
                else
                    cond_bht_counter[update_bht_idx] <=
                        sat_counter_update(
                            cond_bht_counter[update_bht_idx],
                            update_slv.pkt.taken
                        );
            end

            if (update_slv.pkt.vld &&
                update_inst.base.opcode == OPCODE_JALR) begin
                jalr_btb_vld[update_btb_alloc_way][update_btb_set] <= 1'b1;
                jalr_btb_tag[update_btb_alloc_way][update_btb_set] <=
                    update_btb_tag;
                jalr_btb_target_pc[update_btb_alloc_way][update_btb_set] <=
                    update_slv.pkt.target_pc;
                if (!update_btb_hit) begin
                    jalr_btb_replace_way[update_btb_set] <=
                        update_btb_alloc_way + JALR_BTB_WAY_AW'(1);
                end
            end

            if (update_slv.pkt.vld &&
                update_inst.base.opcode == OPCODE_JAL) begin
                if (is_link_reg(update_inst.j.rd)) begin
                    ras_entry[ras_sp] <= update_slv.pkt.pc + 32'd4;
                    ras_sp <= ras_inc(ras_sp);
                    if (ras_count < (RAS_AW + 1)'(RAS_SIZE))
                        ras_count <= ras_count + (RAS_AW + 1)'(1);
                end
            end else if (update_slv.pkt.vld &&
                update_inst.base.opcode == OPCODE_JALR) begin
                if (!is_link_reg(update_inst.i.rd) &&
                    is_link_reg(update_inst.i.rs1)) begin
                    if (ras_count != '0) begin
                        ras_sp <= ras_dec(ras_sp);
                        ras_count <= ras_count - (RAS_AW + 1)'(1);
                    end
                end else if (is_link_reg(update_inst.i.rd) &&
                    !is_link_reg(update_inst.i.rs1)) begin
                    ras_entry[ras_sp] <= update_slv.pkt.pc + 32'd4;
                    ras_sp <= ras_inc(ras_sp);
                    if (ras_count < (RAS_AW + 1)'(RAS_SIZE))
                        ras_count <= ras_count + (RAS_AW + 1)'(1);
                end else if (is_link_reg(update_inst.i.rd) &&
                    is_link_reg(update_inst.i.rs1)) begin
                    if (update_inst.i.rd != update_inst.i.rs1 &&
                        ras_count != '0) begin
                        ras_entry[ras_dec(ras_sp)] <= update_slv.pkt.pc + 32'd4;
                    end else begin
                        ras_entry[ras_sp] <= update_slv.pkt.pc + 32'd4;
                        ras_sp <= ras_inc(ras_sp);
                        if (ras_count < (RAS_AW + 1)'(RAS_SIZE))
                            ras_count <= ras_count + (RAS_AW + 1)'(1);
                    end
                end
            end
        end
    end
endmodule
