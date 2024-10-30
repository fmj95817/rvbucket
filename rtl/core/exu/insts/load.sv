`include "isa.svh"

module load_handler(
    input                  clk,
    input                  rst_n,
    input                  iexec_req_hsk,
    input                  select,
    rv32i_i_dec_if.slave   i_dec,
    exu_dp_if.master       dp_ctrl,
    ldst_if.master         ldst,
    output                 iexec_req_rdy
);
    tri ld_req_pend_flag;
    tri ld_req_pend_set = iexec_req_hsk & select;
    tri ld_req_pend_clear = ldst.rsp_vld & ldst.rsp_rdy;
    lib_flag u_ld_req_pend_flag(clk, rst_n,
        ld_req_pend_set, ld_req_pend_clear, ld_req_pend_flag);

    assign dp_ctrl.gpr_raddr1 = i_dec.rs1;
    assign dp_ctrl.gpr_waddr = i_dec.rd;
    assign dp_ctrl.gpr_wdata = ldst.rsp_pkt.data;
    assign dp_ctrl.gpr_wen = ld_req_pend_flag & ld_req_pend_clear;

    assign dp_ctrl.alu_opcode = ALU_OPCODE_ADD;
    assign dp_ctrl.alu_src1 = dp_ctrl.gpr_rdata1;
    assign dp_ctrl.alu_src2 = i_dec.imm;

    assign iexec_req_rdy = (~ld_req_pend_flag) |
        (ld_req_pend_flag & ld_req_pend_clear);

    assign ldst.req_vld = ld_req_pend_set;
    assign ldst.req_pkt.addr = dp_ctrl.alu_dst;
    assign ldst.req_pkt.st = 1'b0;
endmodule