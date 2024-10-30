
`include "isa.svh"

module exu(
    input          clk,
    input          rst_n,
    iexec_if.slave iexec,
    ldst_if.master ldst_src
);
    tri iexec_req_hsk = iexec.req_vld & iexec.req_rdy;
    tri [`RV_IR_SIZE-1:0] ir = iexec.req_pkt.ir;
    tri [`RV_PC_SIZE-1:0] pc = iexec.req_pkt.pc;

    /* ISA decoder */
    rv32i_opc_dec_if opc_dec();
    rv32i_r_dec_if r_dec();
    rv32i_i_dec_if i_dec();
    rv32i_s_dec_if s_dec();
    rv32i_b_dec_if b_dec();
    rv32i_u_dec_if u_dec();
    rv32i_j_dec_if j_dec();

    rv32i_isa_dec u_rv32i_isa_dec(.*);

    /* data path */
    exu_dp_if dp_ctrl();
    exu_dp u_exu_dp(.*);

    /* instructions */
    exu_dp_if lui_dp_ctrl();
    lui_handler u_lui_handler(iexec_req_hsk, u_dec, lui_dp_ctrl);

    exu_dp_if auipc_dp_ctrl();
    auipc_handler u_auipc_handler(iexec_req_hsk, pc, u_dec, auipc_dp_ctrl);

    exu_dp_if load_dp_ctrl();
    ldst_if load_ldst();
    tri load_iexec_req_rdy;
    load_handler u_load_handler(clk, rst_n, iexec_req_hsk,
        opc_dec.load, i_dec, load_dp_ctrl, load_ldst, load_iexec_req_rdy);

    exu_dp_if alui_dp_ctrl();
    alui_handler u_alui_handler(iexec_req_hsk, i_dec, alui_dp_ctrl);

    /* MUX */
    tri iexec_req_rdy;
    exu_dp_mux u_exu_dp_mux(.*);
    exu_ldst_mux u_exu_ldst_mux(.*);
    exu_iexec_mux u_exu_iexec_mux(.*);

    assign iexec.req_rdy = iexec_req_rdy;
endmodule