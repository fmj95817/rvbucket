`include "isa.svh"

module lui_handler(
    input                iexec_req_hsk,
    rv32i_u_dec_if.slave u_dec,
    exu_dp_if.master     dp_ctrl
);
    assign dp_ctrl.gpr_waddr = u_dec.rd;
    assign dp_ctrl.gpr_wdata = u_dec.imm;
    assign dp_ctrl.gpr_wen = iexec_req_hsk;
endmodule