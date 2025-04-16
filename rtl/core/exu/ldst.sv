`include "isa.svh"

module exu_ldst_handler(
    input logic        clk,
    input logic        rst_n,
    input logic        sel,
    input rv32i_inst_t inst,
    ldst_req_if_t.mst  ldst_req_mst,
    ldst_rsp_if_t.slv  ldst_rsp_slv,
    exu_gpr_if_t.mst   gpr_mst,
    output logic       done
);
    logic ldst_req_pend;

    assign ldst_req_mst.vld = (~ldst_req_pend) & sel;
    assign ldst_rsp_slv.rdy = 1'b1;

    tri ldst_req_hsk = ldst_req_mst.vld & ldst_req_mst.rdy;
    tri ldst_rsp_hsk = ldst_rsp_slv.vld & ldst_rsp_slv.rdy;

    always_ff @(posedge clk or negedge rst_n) begin
        if (~rst_n)
            ldst_req_pend <= 1'b0;
        else if (ldst_req_hsk)
            ldst_req_pend <= 1'b1;
        else if (ldst_rsp_hsk)
            ldst_req_pend <= 1'b0;
    end

    tri is_ld_nxt = (inst.base.opcode == OPCODE_LOAD);
    logic is_ld;
    logic [2:0] ld_funct3;
    logic [`RV_GPR_AW-1:0] ld_rd;

    always_ff @(posedge clk) begin
        if (ldst_req_hsk) begin
            is_ld <= is_ld_nxt;
            ld_funct3 <= inst.i.funct3;
            ld_rd <= inst.i.rd;
        end
    end

    logic [`RV_XLEN-1:0] i_imm;
    logic [`RV_XLEN-1:0] s_imm;
    tri [`RV_XLEN-1:0] offset = is_ld_nxt ? i_imm : s_imm;

    i_imm_decode u_i_imm_decode(
        .inst  (inst),
        .i_imm (i_imm)
    );

    s_imm_decode u_s_imm_decode(
        .inst  (inst),
        .s_imm (s_imm)
    );

    always_comb begin
        if (inst.base.opcode == OPCODE_LOAD) begin
            gpr_mst.ra1 = inst.i.rs1;
            gpr_mst.ra2 = {`RV_GPR_AW{1'b0}};
        end else if (inst.base.opcode == OPCODE_STORE) begin
            gpr_mst.ra1 = inst.s.rs1;
            gpr_mst.ra2 = inst.s.rs2;
        end else begin
            gpr_mst.ra1 = {`RV_GPR_AW{1'bx}};
            gpr_mst.ra2 = {`RV_GPR_AW{1'bx}};
        end
    end

    assign ldst_req_mst.pkt.addr = gpr_mst.rd1 + offset;
    assign ldst_req_mst.pkt.st = ~is_ld_nxt;
    assign ldst_req_mst.pkt.data = gpr_mst.rd2;
    always_comb begin
        case (inst.s.funct3)
            STORE_FUNCT3_SB: ldst_req_mst.pkt.strobe = 4'b0001;
            STORE_FUNCT3_SH: ldst_req_mst.pkt.strobe = 4'b0011;
            STORE_FUNCT3_SW: ldst_req_mst.pkt.strobe = 4'b1111;
            default: ldst_req_mst.pkt.strobe = 4'bxxxx;
        endcase
    end

    tri [`RV_XLEN-1:0] rdata = ldst_rsp_slv.pkt.data;
    assign gpr_mst.wen = sel & is_ld & ldst_rsp_hsk;
    assign gpr_mst.wa = ld_rd;
    always_comb begin
        case (ld_funct3)
            LOAD_FUNCT3_LB: gpr_mst.wd = { {(`RV_XLEN-8){rdata[7]}}, rdata[7:0] };
            LOAD_FUNCT3_LH: gpr_mst.wd = { {(`RV_XLEN-16){rdata[15]}}, rdata[15:0] };
            LOAD_FUNCT3_LW: gpr_mst.wd = rdata;
            LOAD_FUNCT3_LBU: gpr_mst.wd = { {(`RV_XLEN-8){1'b0}}, rdata[7:0] };
            LOAD_FUNCT3_LHU: gpr_mst.wd = { {(`RV_XLEN-16){1'b0}}, rdata[15:0] };
            default: gpr_mst.wd = {`RV_XLEN{1'bx}};
        endcase
    end

    assign done = ldst_rsp_slv.vld & ldst_rsp_slv.rdy;

endmodule