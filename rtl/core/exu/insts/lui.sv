`include "isa.svh"

module lui_handler(
    rv32i_u_dec_if.slave u_dec,
    exu_dp_if.master     dp_op
);
    assign dp_op.gpr_waddr = u_dec.rd;
    assign dp_op.gpr_wdata = u_dec.imm;
    assign dp_op.gpr_wen = 1'b1;
endmodule


