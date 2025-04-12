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

module s_imm_decode(
    input rv32i_inst_t inst,
    output logic [`RV_XLEN-1:0] s_imm
);
    assign s_imm = {
        {(`RV_XLEN-12){inst.s.imm_11_5[6]}},
        inst.s.imm_11_5, inst.s.imm_4_0
    };
endmodule
