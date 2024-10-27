
`include "isa.svh"

module exu(
    input          clk,
    input          rst_n,
    iexec_if.slave iexec,
    ldst_if.master ldst_src
);
    tri iexec_req_hsk = iexec.req_vld & iexec.req_rdy;

    /* ISA decoder */
    tri [`RV_IR_SIZE-1:0] ir = iexec.req_pkt.ir;
    tri [`RV_PC_SIZE-1:0] pc = iexec.req_pkt.pc;

    rv32i_opc_dec_if opc_dec();
    rv32i_r_dec_if r_dec();
    rv32i_i_dec_if i_dec();
    rv32i_s_dec_if s_dec();
    rv32i_b_dec_if b_dec();
    rv32i_u_dec_if u_dec();
    rv32i_j_dec_if j_dec();

    rv32i_isa_dec u_rv32i_isa_dec(.*);

    /* instructions */
    exu_dp_if lui_dp_ctrl();
    lui_handler u_lui_handler(u_dec, lui_dp_ctrl);

    exu_dp_if auipc_dp_ctrl();
    auipc_handler u_auipc_handler(pc, u_dec, auipc_dp_ctrl);

    exu_dp_if alu_imm_dp_ctrl();
    alu_imm_handler u_alu_imm_handler(i_dec, alu_imm_dp_ctrl);

    /* data path */
    exu_dp_if dp_ctrl();
    exu_dp u_exu_dp(.*);
    exu_dp_mux u_exu_dp_mux(.*);

    assign iexec.req_rdy = 1'b1;
    assign ldst_src.req_vld = 1'b0;
endmodule