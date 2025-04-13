`include "isa.svh"

module i_imm_decode(
    input rv32i_inst_t inst,
    output logic [`RV_XLEN-1:0] i_imm
);
    assign i_imm = {
        {(`RV_XLEN-12){inst.i.imm_11_0[11]}},
        inst.i.imm_11_0
    };
endmodule

module j_imm_decode(
    input rv32i_inst_t inst,
    output logic [`RV_XLEN-1:0] j_imm
);
    assign j_imm = {
        {(`RV_XLEN-21){inst.j.imm_20}},
        inst.j.imm_20, inst.j.imm_19_12,
        inst.j.imm_11, inst.j.imm_10_1, 1'b0
    };
endmodule

module b_imm_decode(
    input rv32i_inst_t inst,
    output logic [`RV_XLEN-1:0] b_imm
);
    assign b_imm = {
        {(`RV_XLEN-13){inst.b.imm_12}},
        inst.b.imm_12, inst.b.imm_11,
        inst.b.imm_10_5, inst.b.imm_4_1, 1'b0
    };
endmodule

module s_imm_decode(
    input rv32i_inst_t inst,
    output logic [`RV_XLEN-1:0] s_imm
);
    assign s_imm = {
        {(`RV_XLEN-12){inst.s.imm_11_5[6]}},
        inst.s.imm_11_5, inst.s.imm_4_0
    };
endmodule
